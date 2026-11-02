/* hal.c - GPIO, initialisation, RFID detector and helpers */
#include "hal.h"
#include "hal_priv.h"

#include <string.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "HAL";

i2c_master_bus_handle_t g_hal_i2c_bus = NULL;

static bool              s_relayOn  = false;
static SemaphoreHandle_t s_rfidSem  = NULL;
static volatile bool     s_rfidBusy = false;
static TaskHandle_t      s_detectorTask = NULL;

#define HAL_DETECTOR_PRIORITY   5      /* highest user priority: it only sleeps and polls */
#define HAL_DETECTOR_PERIOD_MS  25

/* ------------------------------------------------------------------ */
static void gpio_out(int pin, int level)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(pin, level);
}

void hal_init(void)
{
    gpio_out(HAL_PIN_LED1, 0);
    gpio_out(HAL_PIN_LED2, 0);
    gpio_out(HAL_PIN_BUZZER, !HAL_BUZZER_ON_LEVEL);
    gpio_out(HAL_PIN_RELAY,  !HAL_RELAY_ON_LEVEL);
    gpio_out(HAL_PIN_SD_CS, 1);                 /* SD card not used: keep it deselected */

    gpio_config_t sw = {
        .pin_bit_mask = 1ULL << HAL_PIN_SW1,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sw);

    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = HAL_PIN_I2C_SDA,
        .scl_io_num = HAL_PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&bus, &g_hal_i2c_bus) != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed");
        return;
    }
    if (hal_oled_init()    != ESP_OK) ESP_LOGW(TAG, "OLED not found at 0x%02X", HAL_OLED_I2C_ADDR);
    if (hal_rtc_init()     != ESP_OK) ESP_LOGW(TAG, "RTC not found at 0x%02X",  HAL_RTC_I2C_ADDR);
    if (hal_mfrc522_init() != ESP_OK) ESP_LOGW(TAG, "RFID reader not responding");
    ESP_LOGI(TAG, "board ready");
}

/* ------------------------------------------------------------------ */
void hal_led_set(uint8_t led, bool on)
{
    gpio_set_level(led == 1 ? HAL_PIN_LED1 : HAL_PIN_LED2, on ? 1 : 0);
}

void hal_buzzer_set(bool on)
{
    gpio_set_level(HAL_PIN_BUZZER, on ? HAL_BUZZER_ON_LEVEL : !HAL_BUZZER_ON_LEVEL);
}

void hal_relay_set(bool on)
{
    s_relayOn = on;
    gpio_set_level(HAL_PIN_RELAY, on ? HAL_RELAY_ON_LEVEL : !HAL_RELAY_ON_LEVEL);
}

bool hal_relay_get(void)      { return s_relayOn; }
bool hal_button_pressed(void) { return gpio_get_level(HAL_PIN_SW1) == 0; }

/* ------------------------------------------------------------------ */
/* Stands in for the RFID interrupt: gives the semaphore when a card is present.
 * The card check yields the CPU while it waits, so it never blocks other tasks. */
static void rfid_detector_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (s_rfidSem != NULL && !s_rfidBusy) {
            if (hal_mfrc522_is_new_card_present()) {
                s_rfidBusy = true;               /* sleep until the card ID has been read */
                xSemaphoreGive(s_rfidSem);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(HAL_DETECTOR_PERIOD_MS));
    }
}

void hal_rfid_set_event_semaphore(SemaphoreHandle_t binarySem)
{
    s_rfidSem = binarySem;
    if (s_detectorTask == NULL) {
        xTaskCreate(rfid_detector_task, "rfid_det", 3072, NULL,
                    HAL_DETECTOR_PRIORITY, &s_detectorTask);
    }
}

bool hal_rfid_read_uid(hal_card_t *card)
{
    bool ok = (card != NULL) && hal_mfrc522_read_uid(card);
    if (ok) hal_mfrc522_halt();   /* the card is ignored until it is removed and shown again */
    s_rfidBusy = false;           /* re-arm the detector */
    return ok;
}

/* ------------------------------------------------------------------ */
void hal_card_to_string(const hal_card_t *card, char *out, size_t outSize)
{
    if (out == NULL || outSize == 0) return;
    out[0] = '\0';
    if (card == NULL) return;
    size_t pos = 0;
    for (uint8_t i = 0; i < card->len && pos + 4 < outSize; i++) {
        pos += snprintf(out + pos, outSize - pos, i ? ":%02X" : "%02X", card->uid[i]);
    }
}

bool hal_card_equals(const hal_card_t *a, const hal_card_t *b)
{
    return a && b && a->len == b->len && memcmp(a->uid, b->uid, a->len) == 0;
}
