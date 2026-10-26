#pragma once
/* MFRC522 (RC522) RFID reader over SPI.
 * Wiring (ESP32 VSPI): SS=5  SCK=18  MOSI=23  MISO=19  RST=4   power 3.3 V ONLY.
 *
 * Only ONE task (RFID_Task) may call these functions, so no mutex is needed:
 * "one device = one owner task" is the simplest way to avoid sharing bugs.   */
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct { uint8_t b[4]; } rc522_uid_t;   /* 4-byte UID (MIFARE Classic 1K) */

esp_err_t rc522_init(void);

/* Look for a NEW card. true  = card found, uid filled, card put to sleep (HALT)
 *                      false = no card / read error.
 * Blocks the calling task for at most ~30 ms (it sleeps with vTaskDelay, it does not spin). */
bool rc522_read_uid(rc522_uid_t *uid);
