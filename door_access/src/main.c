#include <stdio.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hal_oled.h"
#include "hal_rtc.h"
#include "hal_mfrc522.h"
#include "hal_font5x7.h"
#include "hal_priv.h"

#define RELAY GPIO_NUM_2
#define LED GPIO_NUM_4

static SemaphoreHandle_t xRFID_Sem;
static SemaphoreHandle_t xOLED_Mutex;
static QueueHandle_t xData_Queue;
static QueueHandle_t xCommand_Queue;


static void Sensor_Notify(void *pv){ // event notify task a card is here
    xSemaphoreGive(xRFID_Sem);
}

void RFID_Task(void *pv){ // RFID read task
     hal_card_t card;
    int data;
    for(;;){
        if(xSemaphoreTake(xRFID_Sem, portMAX_DELAY) == pdTRUE){
            if(hal_mfrc522_read_uid(&card) ){
                // data = 0;
                // for(int i = 0; i < length.len; i++){
                //     data += card.uid[i];
                // }
                xQueueSend(xData_Queue, &data, portMAX_DELAY);
            }
        }
    }

}

void Access(void *pv){ // Access control task
    int data;
    int status = 0; // 0 for denied, 1 for granted
    for(;;){
        if(xQueueReceive(xData_Queue, &data, portMAX_DELAY) == pdTRUE){
           if( xSemaphoreTake(xOLED_Mutex, portMAX_DELAY) == pdTRUE){
             if(data == 0x1A2B3C4D){ // Example UID sum for access granted
                status = 1;
                hal_oled_show_status("ACCESS GRANTED", "Welcome!");
                printf("ACCESS GRANTED\n");
            }else{
                status = 0;
                hal_oled_show_status("ACCESS DENIED", "Please try again");

                printf("ACCESS DENIED\n");
            }
                xSemaphoreGive(xOLED_Mutex);
           }
            xQueueSend(xCommand_Queue, &status, portMAX_DELAY);
        }
    }
}

void Relay(void *pv){
    int status;
    for(;;){
        if(xQueueReceive(xCommand_Queue, &status, portMAX_DELAY) == pdTRUE){
            if(status == 1){
                gpio_set_level(RELAY, 1); // Turn on relay
                gpio_set_level(LED, 1);   // Turn on LED
                vTaskDelay(pdMS_TO_TICKS(10000)); // Keep relay on for 10 seconds
            }else{
                gpio_set_level(RELAY, 0); // Ensure relay is off 
                gpio_set_level(LED, 0);   // Ensure LED is off     
            }
        }
    }
}

void Clock_Task(void *pv){ // Clock display task
    for(;;){
        hal_time_t current_time;
        if(hal_rtc_get_time(&current_time)){
            if(xSemaphoreTake(xOLED_Mutex, portMAX_DELAY) == pdTRUE){
              hal_oled_show_clock(&current_time);
              // printf("Current Time: %02d:%02d:%02d\n", current_time.hour, current_time.minute, current_time.second);
                xSemaphoreGive(xOLED_Mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
    }
}


void app_main(void)
{
    xRFID_Sem = xSemaphoreCreateBinary();
    xOLED_Mutex = xSemaphoreCreateMutex();
    xData_Queue = xQueueCreate(5, sizeof(int));
    xCommand_Queue = xQueueCreate(1, sizeof(int));

    gpio_set_direction(RELAY, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    hal_init();
    hal_oled_init();
    hal_mfrc522_init();

    //xTaskCreate(Sensor_Notify, "Sensor_Notify", 2048, NULL, 3, NULL);
    xTaskCreate(RFID_Task, "RFID_Task", 4096, NULL, 3, NULL);
    xTaskCreate(Access, "Access", 4096, NULL, 4, NULL);
    xTaskCreate(Relay, "Relay_Task", 4096, NULL, 2, NULL);
    xTaskCreate(Clock_Task, "Clock_Task", 4096, NULL, 1, NULL);

}