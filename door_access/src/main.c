#include<stdio.h>
#include<stdint.h>
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"driver/gpio.h"
#include"freertos/semphr.h"
#include"freertos/queue.h"
#include"esp_random.h"
#include "oled_ssd1306.h"
#include "rc522.h"


#define buzzer 27
#define greenLed 12
#define blueLed 13
#define redLed 14
static SemaphoreHandle_t xCardSem;
static SemaphoreHandle_t xExitSem;
static SemaphoreHandle_t xMutex;
static QueueHandle_t xCardQueue;
static QueueHandle_t xStatusQueue;

typedef struct{
   uint8_t cardID;
   char *name;
} rfidCard_t;

static const rfidCard_t cardList[3] = {
    {0x12, "Ahmed"},
    {0x34, "Sarah"},
    {0x56, "Salah"}
};


void ReadCard(void *pv){
   
  TickType_t last = xTaskGetTickCount();  // last time I was here 

for(;;){
    uint8_t cardID = esp_random() % 256; // generate a random card ID
    xQueueSend(xCardQueue , &cardID , portMAX_DELAY); // send the card ID to the queue
}
    
  
    
}


void app_main() {

    gpio_set_direction(buzzer , GPIO_MODE_OUTPUT);
    gpio_set_direction(greenLed , GPIO_MODE_OUTPUT);
    gpio_set_direction(blueLed , GPIO_MODE_OUTPUT);
    gpio_set_direction(redLed , GPIO_MODE_OUTPUT);

    xCardSem = xSemaphoreCreateBinary();
    xMutex = xSemaphoreCreateMutex();
    xCardQueue = xQueueCreate(4 , sizeof(rc522_uid_t));
   // xStatusQueue = xQueueCreate(3 , sizeof(int));

   



}