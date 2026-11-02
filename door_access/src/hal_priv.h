/* hal_priv.h - internal declarations shared by the HAL source files (not for students) */
#ifndef HAL_PRIV_H
#define HAL_PRIV_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "hal.h"

extern i2c_master_bus_handle_t g_hal_i2c_bus;   /* created in hal_init(), shared by OLED + RTC */

esp_err_t hal_oled_init(void);
esp_err_t hal_rtc_init(void);
esp_err_t hal_mfrc522_init(void);
bool      hal_mfrc522_is_new_card_present(void);   /* REQA                       */
bool      hal_mfrc522_read_uid(hal_card_t *card);  /* anti-collision + select    */
void      hal_mfrc522_halt(void);                  /* HLTA: ignore card until removed */

#endif
