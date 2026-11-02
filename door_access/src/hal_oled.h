#ifndef HAL_OLED_H
#define HAL_OLED_H

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


void hal_oled_show_status(const char *line1, const char *line2);
void hal_oled_show_clock(const hal_time_t *t);
esp_err_t hal_oled_init(void);







#endif