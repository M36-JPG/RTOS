/* hal_mfrc522.c - minimal MFRC522 RFID reader driver over SPI (reads 4-byte card IDs) */
#include "hal_priv.h"

#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_rom_sys.h"
#include "freertos/task.h"

/* MFRC522 registers */
#define CommandReg     0x01
#define ComIrqReg      0x04
#define DivIrqReg      0x05
#define ErrorReg       0x06
#define FIFODataReg    0x09
#define FIFOLevelReg   0x0A
#define ControlReg     0x0C
#define BitFramingReg  0x0D
#define CollReg        0x0E
#define ModeReg        0x11
#define TxModeReg      0x12
#define RxModeReg      0x13
#define TxControlReg   0x14
#define TxASKReg       0x15
#define CRCResultRegH  0x21
#define CRCResultRegL  0x22
#define ModWidthReg    0x24
#define TModeReg       0x2A
#define TPrescalerReg  0x2B
#define TReloadRegH    0x2C
#define TReloadRegL    0x2D
#define VersionReg     0x37

/* commands */
#define CMD_IDLE       0x00
#define CMD_CALCCRC    0x03
#define CMD_TRANSCEIVE 0x0C
#define CMD_SOFTRESET  0x0F

/* card commands */
#define PICC_REQA      0x26
#define PICC_SEL_CL1   0x93
#define PICC_HLTA      0x50

static spi_device_handle_t s_spi = NULL;

/* ---------------- register access ---------------- */
static uint8_t rc_read(uint8_t reg)
{
    uint8_t tx[2] = { (uint8_t)(((reg << 1) & 0x7E) | 0x80), 0x00 };
    uint8_t rx[2] = { 0, 0 };
    spi_transaction_t t = { .length = 16, .tx_buffer = tx, .rx_buffer = rx };
    spi_device_polling_transmit(s_spi, &t);
    return rx[1];
}

static void rc_write(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { (uint8_t)((reg << 1) & 0x7E), val };
    spi_transaction_t t = { .length = 16, .tx_buffer = tx };
    spi_device_polling_transmit(s_spi, &t);
}

static void rc_setbits(uint8_t reg, uint8_t mask) { rc_write(reg, rc_read(reg) | mask); }
static void rc_clrbits(uint8_t reg, uint8_t mask) { rc_write(reg, rc_read(reg) & (uint8_t)~mask); }

/* ---------------- CRC and transceive ---------------- */
static bool rc_calc_crc(const uint8_t *data, uint8_t len, uint8_t out[2])
{
    rc_write(CommandReg, CMD_IDLE);
    rc_write(DivIrqReg, 0x04);                 /* clear CRCIRq   */
    rc_write(FIFOLevelReg, 0x80);              /* flush FIFO     */
    for (uint8_t i = 0; i < len; i++) rc_write(FIFODataReg, data[i]);
    rc_write(CommandReg, CMD_CALCCRC);
    for (int i = 0; i < 200; i++) {
        if (rc_read(DivIrqReg) & 0x04) {
            rc_write(CommandReg, CMD_IDLE);
            out[0] = rc_read(CRCResultRegL);
            out[1] = rc_read(CRCResultRegH);
            return true;
        }
        esp_rom_delay_us(50);
    }
    return false;
}

/* Sends tx[] to the card and collects the answer in rx[].
 * Waits with short delays first, then yields the CPU (vTaskDelay), so a slow or
 * missing card never blocks other tasks. rxLen: in = buffer size, out = bytes read. */
static bool rc_transceive(const uint8_t *tx, uint8_t txLen, uint8_t *rx, uint8_t *rxLen,
                          uint8_t txLastBits)
{
    rc_write(CommandReg, CMD_IDLE);
    rc_write(ComIrqReg, 0x7F);                 /* clear all interrupt flags */
    rc_write(FIFOLevelReg, 0x80);              /* flush FIFO                */
    for (uint8_t i = 0; i < txLen; i++) rc_write(FIFODataReg, tx[i]);
    rc_write(BitFramingReg, txLastBits);
    rc_write(CommandReg, CMD_TRANSCEIVE);
    rc_setbits(BitFramingReg, 0x80);           /* StartSend */

    bool done = false;
    for (int i = 0; i < 60; i++) {
        uint8_t irq = rc_read(ComIrqReg);
        if (irq & 0x30) { done = true; break; }  /* RxIRq or IdleIRq     */
        if (irq & 0x01) break;                   /* TimerIRq = no answer */
        if (i < 8) esp_rom_delay_us(100); else vTaskDelay(1);
    }
    rc_clrbits(BitFramingReg, 0x80);
    if (!done) return false;
    if (rc_read(ErrorReg) & 0x13) return false;  /* buffer overflow, parity, protocol */

    if (rx != NULL && rxLen != NULL) {
        uint8_t n = rc_read(FIFOLevelReg);
        if (n > *rxLen) return false;
        *rxLen = n;
        for (uint8_t i = 0; i < n; i++) rx[i] = rc_read(FIFODataReg);
    }
    return true;
}

/* ---------------- public ---------------- */
esp_err_t hal_mfrc522_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = HAL_PIN_SPI_MOSI,
        .miso_io_num = HAL_PIN_SPI_MISO,
        .sclk_io_num = HAL_PIN_SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED);
    if (err != ESP_OK) return err;

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = HAL_PIN_RFID_SS,
        .queue_size = 1,
    };
    err = spi_bus_add_device(SPI2_HOST, &dev, &s_spi);
    if (err != ESP_OK) return err;

    /* hardware reset with the RST pin, then soft reset */
    gpio_config_t rst = { .pin_bit_mask = 1ULL << HAL_PIN_RFID_RST, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&rst);
    gpio_set_level(HAL_PIN_RFID_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(HAL_PIN_RFID_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    rc_write(CommandReg, CMD_SOFTRESET);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* timer: card must answer within about 5 ms */
    rc_write(TModeReg, 0x80);
    rc_write(TPrescalerReg, 0xA9);
    rc_write(TReloadRegH, 0x00);
    rc_write(TReloadRegL, 0xC8);
    rc_write(TxASKReg, 0x40);                  /* 100 % ASK modulation */
    rc_write(ModeReg, 0x3D);                   /* CRC preset 0x6363    */
    rc_setbits(TxControlReg, 0x03);            /* antenna on           */

    uint8_t v = rc_read(VersionReg);
    return (v == 0x00 || v == 0xFF) ? ESP_ERR_NOT_FOUND : ESP_OK;
}

bool hal_mfrc522_is_new_card_present(void)
{
    uint8_t cmd = PICC_REQA, atqa[2], n = sizeof(atqa);
    rc_write(TxModeReg, 0x00);
    rc_write(RxModeReg, 0x00);
    rc_write(ModWidthReg, 0x26);
    rc_clrbits(CollReg, 0x80);
    return rc_transceive(&cmd, 1, atqa, &n, 7 /* REQA is a 7-bit frame */) && n == 2;
}

bool hal_mfrc522_read_uid(hal_card_t *card)
{
    uint8_t buf[9], rx[5], n = sizeof(rx);

    /* anti-collision, cascade level 1 */
    buf[0] = PICC_SEL_CL1;  buf[1] = 0x20;
    rc_clrbits(CollReg, 0x80);
    if (!rc_transceive(buf, 2, rx, &n, 0) || n != 5) return false;
    if ((uint8_t)(rx[0] ^ rx[1] ^ rx[2] ^ rx[3]) != rx[4]) return false;
    if (rx[0] == 0x88) return false;           /* 7-byte card IDs are not supported */

    /* select the card */
    buf[0] = PICC_SEL_CL1;  buf[1] = 0x70;
    memcpy(&buf[2], rx, 5);                    /* 4 ID bytes + BCC */
    if (!rc_calc_crc(buf, 7, &buf[7])) return false;
    uint8_t sak[3], sn = sizeof(sak);
    if (!rc_transceive(buf, 9, sak, &sn, 0) || sn != 3) return false;

    memcpy(card->uid, rx, 4);
    card->len = 4;
    return true;
}

void hal_mfrc522_halt(void)
{
    uint8_t buf[4] = { PICC_HLTA, 0x00, 0, 0 };
    if (rc_calc_crc(buf, 2, &buf[2])) {
        rc_transceive(buf, 4, NULL, NULL, 0);  /* the card does not answer; a timeout is normal */
    }
}
