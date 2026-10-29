#include<stdio.h>
#include<stdint.h>
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"driver/gpio.h"
#include"freertos/semphr.h"
#include"freertos/queue.h"
#include"esp_random.h"

#define buzzer 27
#define Led 12
#define relay 14

static SemaphoreHandle_t xCardSem;
static QueueHandle_t xCardQueue;



void ReadCard(void *pv){
   
  TickType_t last = xTaskGetTickCount();  // last time I was here
  for(;;){
    uint8_t cardID = esp_random() % 256; // generate a random card ID
    xQueueSend(xCardQueue , &cardID , portMAX_DELAY); // send the card ID to the queue
  }
}

void Feedback(void *pv){
    for(;;){
        uint8_t cardID;
        xQueueReceive(xCardQueue , &cardID , portMAX_DELAY); // receive the card ID from the queue
        xSemaphoreTake(xCardSem, portMAX_DELAY); // take the semaphore to signal that a card has been read
        if(cardID == 0x12){
             printf("ACCESS GRANTED\n");
              gpio_set_level(Led, 1); // turn on the LED
              gpio_set_level(relay, 1); // turn on the relay
              vTaskDelay(pdMS_TO_TICKS(10000)); // wait for 5 seconds
              gpio_set_level(Led, 0); // turn off the LED
              gpio_set_level(relay, 0); // turn off the relay
        }
        else if(cardID == 0x34){
             printf("ACCESS GRANTED\n");
              gpio_set_level(Led, 1); // turn on the LED
              gpio_set_level(relay, 1); // turn on the relay
              vTaskDelay(pdMS_TO_TICKS(10000)); // wait for 5 seconds
              gpio_set_level(Led, 0); // turn off the LED
              gpio_set_level(relay, 0); // turn off the relay
        }
        else if(cardID == 0x56){
             printf("ACCESS GRANTED\n");
              gpio_set_level(Led, 1); // turn on the LED
              gpio_set_level(relay, 1); // turn on the relay
              vTaskDelay(pdMS_TO_TICKS(10000)); // wait for 5 seconds
              gpio_set_level(Led, 0); // turn off the LED
              gpio_set_level(relay, 0); // turn off the relay
        }
        else{
           printf("Unknown card detected\n"); // perform action for unknown card
              gpio_set_level(Led, 0); // turn off the LED
              gpio_set_level(buzzer, 1); // turn on the buzzer
              vTaskDelay(pdMS_TO_TICKS(2000)); // wait for 2 seconds
              gpio_set_level(buzzer, 0); // turn off the buzzer
              printf("ACCESS DENIED\n");
        }
        xSemaphoreGive(xCardSem); // give the semaphore to signal that the card has been processed
    }
  }

void Clock_Update(void *pv){
    TickType_t last = xTaskGetTickCount();  // last time I was here
    for(;;){
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000)); // wait for 1 second
        printf("Clock updated\n"); // perform action for clock update
    }
}



void app_main() {


    xCardSem = xSemaphoreCreateBinary();
    xCardQueue = xQueueCreate(4 , sizeof(uint8_t));
    gpio_set_direction(buzzer , GPIO_MODE_OUTPUT);
    gpio_set_direction(Led , GPIO_MODE_OUTPUT);
    gpio_set_direction(relay , GPIO_MODE_OUTPUT);

    xTaskCreate(ReadCard , "ReadCard" , 2048 , NULL , 2 , NULL);
    xTaskCreate(Feedback , "Feedback" , 2048 , NULL , 3 , NULL);
    xTaskCreate(Clock_Update , "Clock_Update" , 2048 , NULL , 1 , NULL);
}