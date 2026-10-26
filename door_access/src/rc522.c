#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rc522.h"

#define TAG "rc522"
#define PIN_SS   5
#define PIN_SCK  18
#define PIN_MOSI 23
#define PIN_MISO 19
#define PIN_RST  4

/* --- RC522 registers --- */
#define CommandReg     0x01
#define ComIEnReg      0x02
#define ComIrqReg      0x04
#define DivIrqReg      0x05
#define ErrorReg       0x06
#define FIFODataReg    0x09
#define FIFOLevelReg   0x0A
#define ControlReg     0x0C
#define BitFramingReg  0x0D
#define CollReg        0x0E
#define ModeReg        0x11
#define TxControlReg   0x14
#define TxASKReg       0x15
#define CRCResultRegH  0x21
#define CRCResultRegL  0x22
#define TModeReg       0x2A
#define TPrescalerReg  0x2B
#define TReloadRegH    0x2C
#define TReloadRegL    0x2D
#define VersionReg     0x37

/* --- RC522 commands (written to CommandReg) --- */
#define CMD_IDLE        0x00
#define CMD_CALCCRC     0x03
#define CMD_TRANSCEIVE  0x0C
#define CMD_SOFTRESET   0x0F

/* --- Card (PICC) commands --- */
#define PICC_REQA     0x26
#define PICC_SEL_CL1  0x93
#define PICC_HLTA     0x50

static spi_device_handle_t s_dev;

/* ---------- SPI register access ----------
 * First byte = (reg << 1) with bit7 = 1 for READ, 0 for WRITE. Bit0 must be 0.      */
static uint8_t rd(uint8_t reg)
{
    uint8_t tx[2] = { (uint8_t)(0x80 | (reg << 1)), 0x00 }, rx[2] = { 0, 0 };
    spi_transaction_t t = { .length = 16, .tx_buffer = tx, .rx_buffer = rx };
    spi_device_polling_transmit(s_dev, &t);
    return rx[1];
}
static void wr(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { (uint8_t)(reg << 1), val };
    spi_transaction_t t = { .length = 16, .tx_buffer = tx };
    spi_device_polling_transmit(s_dev, &t);
}
static void set_bits(uint8_t reg, uint8_t m) { wr(reg, rd(reg) | m); }
static void clr_bits(uint8_t reg, uint8_t m) { wr(reg, rd(reg) & (uint8_t)~m); }

/* Wait until (reg & mask) != 0. Sleeps 1 tick between polls so other tasks can run. */
static bool wait_reg(uint8_t reg, uint8_t mask, int ms)
{
    int64_t end = esp_timer_get_time() + (int64_t)ms * 1000;
    do {
        if (rd(reg) & mask) return true;
        vTaskDelay(1);
    } while (esp_timer_get_time() < end);
    return false;
}

/* ---------- CRC_A done by the RC522 co-processor ---------- */
static bool calc_crc(const uint8_t *d, uint8_t n, uint8_t out[2])
{
    wr(CommandReg, CMD_IDLE);
    wr(DivIrqReg, 0x04);            /* clear CRCIRq   */
    wr(FIFOLevelReg, 0x80);         /* flush FIFO     */
    for (uint8_t i = 0; i < n; i++) wr(FIFODataReg, d[i]);
    wr(CommandReg, CMD_CALCCRC);
    if (!wait_reg(DivIrqReg, 0x04, 20)) return false;
    wr(CommandReg, CMD_IDLE);
    out[0] = rd(CRCResultRegL);
    out[1] = rd(CRCResultRegH);
    return true;
}

/* ---------- Send bytes to the card and read the answer ----------
 * return: 0 = OK, -1 = timeout (no card answered), -2 = RF error (parity/collision/overflow)
 * tx_last_bits: 7 for the short REQA frame, 0 for normal bytes.                        */
static int transceive(const uint8_t *tx, uint8_t tx_len, uint8_t *rx, uint8_t *rx_len, uint8_t tx_last_bits)
{
    wr(CommandReg, CMD_IDLE);
    wr(ComIEnReg, 0x77 | 0x80);
    wr(ComIrqReg, 0x7F);            /* clear all IRQ flags */
    wr(FIFOLevelReg, 0x80);         /* flush FIFO          */
    for (uint8_t i = 0; i < tx_len; i++) wr(FIFODataReg, tx[i]);
    wr(BitFramingReg, tx_last_bits);
    wr(CommandReg, CMD_TRANSCEIVE);
    set_bits(BitFramingReg, 0x80);  /* StartSend: go! */

    /* bit5 RxIRq | bit4 IdleIRq = answer received,  bit0 TimerIRq = card never answered */
    bool done = wait_reg(ComIrqReg, 0x30 | 0x01, 30);
    clr_bits(BitFramingReg, 0x80);
    uint8_t irq = rd(ComIrqReg);

    if (!done || (irq & 0x01)) return -1;
    if (rd(ErrorReg) & 0x1B)    return -2;      /* BufferOvfl | Coll | Parity | Protocol */

    uint8_t n = rd(FIFOLevelReg);
    if (rx && rx_len) {
        if (n > *rx_len) return -2;
        for (uint8_t i = 0; i < n; i++) rx[i] = rd(FIFODataReg);
        *rx_len = n;
    }
    return 0;
}

esp_err_t rc522_init(void)
{
    gpio_config_t rst = { .pin_bit_mask = 1ULL << PIN_RST, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&rst);
    gpio_set_level(PIN_RST, 0); vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1); vTaskDelay(pdMS_TO_TICKS(50));   /* oscillator start-up */

    spi_bus_config_t bus = {
        .mosi_io_num = PIN_MOSI, .miso_io_num = PIN_MISO, .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 16,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev = {
        .clock_speed_hz = 5 * 1000 * 1000,   /* RC522 max is 10 MHz */
        .mode = 0,                           /* CPOL=0 CPHA=0       */
        .spics_io_num = PIN_SS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &dev, &s_dev));

    wr(CommandReg, CMD_SOFTRESET);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t ver = rd(VersionReg);
    ESP_LOGI(TAG, "VersionReg = 0x%02X (0x91/0x92 = genuine chip, 0x88/0x12 = clone)", ver);
    if (ver == 0x00 || ver == 0xFF) {
        ESP_LOGE(TAG, "RC522 not answering: check wiring (SS=5 SCK=18 MOSI=23 MISO=19) and 3.3 V");
        return ESP_ERR_NOT_FOUND;
    }

    wr(TModeReg, 0x80);        /* timer starts automatically after transmit    */
    wr(TPrescalerReg, 0xA9);   /* timer tick = 25 us                           */
    wr(TReloadRegH, 0x03);
    wr(TReloadRegL, 0xE8);     /* 1000 ticks x 25 us = 25 ms card-answer timeout */
    wr(TxASKReg, 0x40);        /* 100 % ASK modulation                         */
    wr(ModeReg, 0x3D);         /* CRC preset 0x6363                            */
    set_bits(TxControlReg, 0x03);   /* antenna ON (Tx1 + Tx2)                  */
    return ESP_OK;
}

bool rc522_read_uid(rc522_uid_t *uid)
{
    uint8_t buf[9], n;

    /* 1) REQA: "is any card there?"  A sleeping (HALT) card does NOT answer REQA. */
    clr_bits(CollReg, 0x80);
    buf[0] = PICC_REQA; n = 2;
    uint8_t atqa[2]; uint8_t an = 2;
    if (transceive(buf, 1, atqa, &an, 7) != 0 || an != 2) return false;

    /* 2) Anti-collision: card answers UID(4) + BCC(1) */
    uint8_t ac[2] = { PICC_SEL_CL1, 0x20 }; uint8_t r[5]; n = sizeof r;
    if (transceive(ac, 2, r, &n, 0) != 0 || n != 5) return false;
    if ((r[0] ^ r[1] ^ r[2] ^ r[3]) != r[4]) return false;           /* BCC must match */
    if (r[0] == 0x88) { ESP_LOGW(TAG, "7-byte UID card not supported"); return false; }

    /* 3) SELECT the card (UID + BCC + CRC) so it becomes ACTIVE */
    buf[0] = PICC_SEL_CL1; buf[1] = 0x70;
    memcpy(&buf[2], r, 5);
    if (!calc_crc(buf, 7, &buf[7])) return false;
    uint8_t sak[3]; n = sizeof sak;
    if (transceive(buf, 9, sak, &n, 0) != 0 || n != 3) return false;

    memcpy(uid->b, r, 4);

    /* 4) HALT: card goes to sleep, so the 100 ms polling does not report it again
     *    while it stays on the reader. (No answer is expected -> timeout is fine.)  */
    uint8_t h[4] = { PICC_HLTA, 0x00 };
    if (calc_crc(h, 2, &h[2])) transceive(h, 4, NULL, NULL, 0);
    return true;
}
