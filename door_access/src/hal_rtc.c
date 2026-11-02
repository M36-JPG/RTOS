#include <stdio.h>
/* hal_rtc.c - DS3231 / DS1307 real-time clock over I2C (same register layout) */
#include "hal_priv.h"
#include <string.h>

#define I2C_TIMEOUT_MS 50

static i2c_master_dev_handle_t s_dev = NULL;

static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

bool hal_rtc_get_time(hal_time_t *t)
{
    uint8_t reg = 0x00, d[7];
    if (t == NULL || s_dev == NULL) return false;
    if (i2c_master_transmit_receive(s_dev, &reg, 1, d, sizeof(d), I2C_TIMEOUT_MS) != ESP_OK)
        return false;
    t->second = bcd2bin(d[0] & 0x7F);              /* bit 7 = clock halt (DS1307) */
    t->minute = bcd2bin(d[1] & 0x7F);
    if (d[2] & 0x40) {                             /* 12-hour mode */
        uint8_t h = bcd2bin(d[2] & 0x1F) % 12;
        t->hour = (d[2] & 0x20) ? h + 12 : h;
    } else {
        t->hour = bcd2bin(d[2] & 0x3F);
    }
    t->day   = bcd2bin(d[4] & 0x3F);
    t->month = bcd2bin(d[5] & 0x1F);
    t->year  = 2000 + bcd2bin(d[6]);
    return true;
}

bool hal_rtc_set_time(const hal_time_t *t)
{
    if (t == NULL || s_dev == NULL) return false;
    uint8_t buf[8] = {
        0x00,                                       /* start at register 0        */
        bin2bcd(t->second),                         /* bit 7 = 0 starts a DS1307  */
        bin2bcd(t->minute),
        bin2bcd(t->hour),                           /* 24-hour mode               */
        1,                                          /* day of week (not used)     */
        bin2bcd(t->day),
        bin2bcd(t->month),
        bin2bcd((uint8_t)(t->year % 100)),
    };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), I2C_TIMEOUT_MS) == ESP_OK;
}

/* Build time from __DATE__ ("Mmm dd yyyy") and __TIME__ ("hh:mm:ss") */
static void build_time(hal_time_t *t)
{
    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {0};
    int d = 1, y = 2026, hh = 0, mm = 0, ss = 0;
    sscanf(__DATE__, "%3s %d %d", mon, &d, &y);
    sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
    const char *p = strstr(months, mon);
    t->month = p ? (uint8_t)((p - months) / 3 + 1) : 1;
    t->day = (uint8_t)d;  t->year = (uint16_t)y;
    t->hour = (uint8_t)hh; t->minute = (uint8_t)mm; t->second = (uint8_t)ss;
}

esp_err_t hal_rtc_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = HAL_RTC_I2C_ADDR,
        .scl_speed_hz    = 400000,
    };
    esp_err_t err = i2c_master_bus_add_device(g_hal_i2c_bus, &cfg, &s_dev);
    if (err != ESP_OK) return err;

    hal_time_t t;
    if (!hal_rtc_get_time(&t)) return ESP_FAIL;     /* no ACK: RTC missing */
    if (t.year < 2024) {                            /* never set: use the build time */
        build_time(&t);
        hal_rtc_set_time(&t);
    }
    return ESP_OK;
}
