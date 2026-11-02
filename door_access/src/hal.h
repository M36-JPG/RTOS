/*
 * hal.h - Hardware Abstraction Layer for the ESP32 IOT Development Board V2.0
 * Course : Designs and Deploys Real-Time Operating Systems (ECU-SD-5-02)
 *
 * Framework : ESP-IDF v5.2 or newer (plain C, no extra libraries)
 *             FreeRTOS: 1 ms tick, preemptive, time slicing ON, single core
 *             (see sdkconfig.defaults)
 *
 * Pin map taken from the "Peripherals GPIO Reference" printed on the board.
 *
 * IMPORTANT RULES FOR STUDENTS
 *  - The OLED and the RTC share ONE I2C bus (SDA = GPIO21, SCL = GPIO22).
 *    hal_rtc_get_time() and every hal_oled_*() call use that bus and are NOT
 *    protected for you. Protect them with ONE mutex (requirement R5).
 */
#ifndef HAL_H
#define HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- Pin map (from the board label) ---------------- */
#define HAL_PIN_LED1        12   /* LED1  - use as the "system active" status LED */
#define HAL_PIN_LED2        14   /* LED2                                          */
#define HAL_PIN_SW1         26   /* SW1 push button (active low)                  */
#define HAL_PIN_BUZZER      27   /* Buzzer                                        */
#define HAL_PIN_RELAY       13   /* Relay module (classroom lights)               */

#define HAL_PIN_I2C_SDA     21   /* RTC and OLED share this bus (D21)             */
#define HAL_PIN_I2C_SCL     22   /* (D22)                                         */

#define HAL_PIN_RFID_RST     5   /* RFID RST                                      */
#define HAL_PIN_SPI_MISO    19   /* RFID / SD MISO                                */
#define HAL_PIN_SPI_MOSI    23   /* RFID / SD MOSI                                */
#define HAL_PIN_SPI_SCK     18   /* RFID / SD SCK                                 */
#define HAL_PIN_RFID_SS      2   /* RFID SDA (= chip select)                      */
#define HAL_PIN_SD_CS        4   /* SD card chip select (kept HIGH = unused)      */
/* RS485 TX/RX uses UART0 and is not used in this course. */

/* ---------------- Board settings you may need to change ---------------- */
#define HAL_RELAY_ON_LEVEL   1      /* set to 0 if your relay module is active-low */
#define HAL_BUZZER_ON_LEVEL  1
#define HAL_OLED_I2C_ADDR    0x3C   /* some OLEDs use 0x3D                         */
#define HAL_RTC_I2C_ADDR     0x68   /* DS3231 and DS1307 both use 0x68             */

/* ---------------- Data types ---------------- */
typedef struct {
    uint8_t uid[10];
    uint8_t len;            /* this HAL reads 4-byte card IDs */
} hal_card_t;

typedef struct {
    uint16_t year;
    uint8_t  month, day;
    uint8_t  hour, minute, second;
} hal_time_t;

/* ---------------- General ---------------- */
void hal_init(void);   /* GPIO, I2C, OLED, RTC and RFID reader. Call once, first. */

/* ---------------- Simple outputs / inputs (safe to call from any task) ---- */
void hal_led_set(uint8_t led, bool on);   /* led = 1 or 2 */
void hal_buzzer_set(bool on);
void hal_relay_set(bool on);
bool hal_relay_get(void);
bool hal_button_pressed(void);            /* SW1, true while pressed */

/* ---------------- I2C devices (NOT protected - take your mutex first) ---- */
bool hal_rtc_get_time(hal_time_t *t);         /* about 1 ms on the bus                  */
bool hal_rtc_set_time(const hal_time_t *t);

/* The OLED screen has two parts: a status area (2 lines) and a clock area.
 * Each call below stores its part, redraws the screen and sends only the parts
 * that changed to the display (a clock update takes about 12 ms on the bus).
 * The status text stays on the screen until it is replaced. */
void hal_oled_show_status(const char *line1, const char *line2);
void hal_oled_show_clock(const hal_time_t *t);

/* ---------------- RFID reader (SPI) ---------------- */
/* The V2.0 board does not route the RFID reader IRQ pin to the ESP32, so the HAL
 * runs a small high-priority "detector" task that stands in for the RFID
 * interrupt: when a card is present it GIVES the binary semaphore you pass in.
 * After the semaphore is given, call hal_rfid_read_uid() to get the card ID. */
void hal_rfid_set_event_semaphore(SemaphoreHandle_t binarySem);
bool hal_rfid_read_uid(hal_card_t *card); /* true = card ID read. Re-arms the detector. */

/* ---------------- Helpers ---------------- */
void hal_card_to_string(const hal_card_t *card, char *out, size_t outSize); /* "A1:B2:C3:D4" */
bool hal_card_equals(const hal_card_t *a, const hal_card_t *b);

#ifdef __cplusplus
}
#endif
#endif /* HAL_H */
