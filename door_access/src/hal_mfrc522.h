#ifndef HAL_MFRC522_H
#define HAL_MFRC522_H

bool hal_mfrc522_is_new_card_present(void);
bool hal_mfrc522_read_uid(hal_card_t *card);
void hal_mfrc522_halt(void);
esp_err_t hal_mfrc522_init(void);

#endif