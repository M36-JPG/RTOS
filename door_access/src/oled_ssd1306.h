#pragma once
/* SSD1306 128x64 OLED over I2C (ESP32: SDA=21, SCL=22, address 0x3C).
 *
 * The driver keeps a 1 KB frame buffer in RAM.
 *   oled_text()  -> only changes RAM (fast, no I2C)
 *   oled_flush() -> sends RAM to the screen (about 25 ms on the bus)
 * The frame buffer is SHARED DATA, so when two tasks use the OLED
 * they must wrap "draw + flush" in ONE mutex lock (done in main.c).
 *
 * Text layout: 8 rows (0..7) x 21 columns, font 5x7 (6 px per char).
 */
#include <stdint.h>
#include "esp_err.h"

#define OLED_COLS 21
#define OLED_ROWS 8

esp_err_t oled_init(void);
void      oled_clear(void);
void      oled_clear_row(uint8_t row);
void      oled_text(uint8_t col, uint8_t row, const char *s);
esp_err_t oled_flush(void);
