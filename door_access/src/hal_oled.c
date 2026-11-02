/* hal_oled.c - minimal SSD1306 128x64 I2C driver (frame buffer + 5x8 text) */
#include "hal_priv.h"
#include "hal_font5x7.h"

#include <stdio.h>
#include <string.h>

#define OLED_W 128
#define OLED_H 64
#define OLED_PAGES (OLED_H / 8)
#define I2C_TIMEOUT_MS 100

static i2c_master_dev_handle_t s_dev = NULL;
static uint8_t s_fb[OLED_W * OLED_PAGES];     /* frame being drawn          */
static uint8_t s_sent[OLED_W * OLED_PAGES];   /* what the display shows now */

static char s_status1[24] = "";
static char s_status2[24] = "";
static char s_date[16]    = "--/--/----";
static char s_time[16]    = "--:--:--";

/* ---------------- low level ---------------- */
static esp_err_t oled_cmds(const uint8_t *cmds, size_t n)
{
    uint8_t buf[32];
    if (n > sizeof(buf) - 1) return ESP_ERR_INVALID_SIZE;
    buf[0] = 0x00;                                   /* control byte: commands */
    memcpy(buf + 1, cmds, n);
    return i2c_master_transmit(s_dev, buf, n + 1, I2C_TIMEOUT_MS);
}

/* Sends only the pages (8-row strips) that changed since the last frame.
 * One page = 128 data bytes = about 3 ms at 400 kHz, so a clock update (about
 * 4 pages) takes about 12 ms and the mutex is held for a short time only. */
static esp_err_t oled_flush(void)
{
    uint8_t chunk[OLED_W + 1];
    chunk[0] = 0x40;                                 /* control byte: data */
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        if (memcmp(&s_fb[page * OLED_W], &s_sent[page * OLED_W], OLED_W) == 0) continue;
        const uint8_t window[] = { 0x21, 0, OLED_W - 1,   /* column range   */
                                   0x22, page, page };    /* this page only */
        esp_err_t err = oled_cmds(window, sizeof(window));
        if (err != ESP_OK) return err;
        memcpy(chunk + 1, &s_fb[page * OLED_W], OLED_W);
        err = i2c_master_transmit(s_dev, chunk, sizeof(chunk), I2C_TIMEOUT_MS);
        if (err != ESP_OK) return err;
        memcpy(&s_sent[page * OLED_W], &s_fb[page * OLED_W], OLED_W);
    }
    return ESP_OK;
}

esp_err_t hal_oled_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = HAL_OLED_I2C_ADDR,
        .scl_speed_hz    = 400000,
    };
    esp_err_t err = i2c_master_bus_add_device(g_hal_i2c_bus, &cfg, &s_dev);
    if (err != ESP_OK) return err;

    static const uint8_t init[] = {
        0xAE,             /* display off                        */
        0xD5, 0x80,       /* clock divide                       */
        0xA8, 0x3F,       /* multiplex 64                       */
        0xD3, 0x00,       /* display offset 0                   */
        0x40,             /* start line 0                       */
        0x8D, 0x14,       /* charge pump on                     */
        0x20, 0x00,       /* horizontal addressing mode         */
        0xA1,             /* segment remap                      */
        0xC8,             /* COM scan direction remapped        */
        0xDA, 0x12,       /* COM pins                           */
        0x81, 0xCF,       /* contrast                           */
        0xD9, 0xF1,       /* pre-charge                         */
        0xDB, 0x40,       /* VCOMH                              */
        0xA4,             /* follow RAM content                 */
        0xA6,             /* normal (not inverted)              */
        0xAF,             /* display on                         */
    };
    err = oled_cmds(init, sizeof(init));
    if (err != ESP_OK) return err;
    /* clear the whole display once: send an all-zero frame */
    memset(s_fb, 0, sizeof(s_fb));
    memset(s_sent, 0xFF, sizeof(s_sent));            /* force every page to be sent */
    return oled_flush();
}

/* ---------------- drawing ---------------- */
static void px(int x, int y)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    s_fb[(y >> 3) * OLED_W + x] |= (uint8_t)(1u << (y & 7));
}

static void draw_text(int x, int y, const char *s, int size)
{
    for (; *s; s++, x += 6 * size) {
        char c = *s;
        if (c < 32 || c > 126) c = '?';
        const uint8_t *g = hal_font5x7[c - 32];
        for (int col = 0; col < 5; col++)
            for (int row = 0; row < 8; row++)
                if (g[col] & (1u << row))
                    for (int dx = 0; dx < size; dx++)
                        for (int dy = 0; dy < size; dy++)
                            px(x + col * size + dx, y + row * size + dy);
    }
}

static void render(void)
{
    memset(s_fb, 0, sizeof(s_fb));
    draw_text(0, 0,  s_status1, 1);
    draw_text(0, 12, s_status2, 1);
    for (int x = 0; x < OLED_W; x++) px(x, 28);       /* separator line */
    draw_text(0, 34, s_date, 1);
    draw_text(0, 46, s_time, 2);
    oled_flush();                                      /* send changed pages only */
}

/* ---------------- public ---------------- */
void hal_oled_show_status(const char *line1, const char *line2)
{
    snprintf(s_status1, sizeof(s_status1), "%s", line1 ? line1 : "");
    snprintf(s_status2, sizeof(s_status2), "%s", line2 ? line2 : "");
    render();
}

void hal_oled_show_clock(const hal_time_t *t)
{
    if (t != NULL) {
        snprintf(s_date, sizeof(s_date), "%02u/%02u/%04u", t->day, t->month, t->year);
        snprintf(s_time, sizeof(s_time), "%02u:%02u:%02u", t->hour, t->minute, t->second);
    }
    render();
}
