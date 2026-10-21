#include<stdio.h>
#include<stdarg.h>
#include<stdint.h>
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"driver/gpio.h"
#include"freertos/semphr.h"
#include"freertos/queue.h"
#include"esp_random.h"


#define TempSensor 15
#define  TEMP_Limit 70
#define buzzer 27
static SemaphoreHandle_t xLogMutex;
static SemaphoreHandle_t xAlarmSem;
static QueueHandle_t xSensorQueue;


void ReadTemp(void *pvParameter){ // generic pointer 

    TickType_t last = xTaskGetTickCount(); // refernce point to know where was this tick 

    for(;;){
        int value = 50 + esp_random() % 40 ;
        xQueueSend(xSensorQueue , &value , 0); // 0 for queue overflow
        vTaskDelayUntil(&last , pdMS_TO_TICKS(100)); //delayUntil 
    }
}


void Alarm(void *pv){

    for(;;){
        xSemaphoreTake(xAlarmSem , portMAX_DELAY);
        gpio_set_level(buzzer , 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(buzzer , 0);
    }

}

void Process (void *pv){
    int sample , sum =0 , n =0;
    for(;;){
        xQueueReceive(xSensorQueue , &sample , portMAX_DELAY);
        sum+= sample;
        if(++n == 5){
            if(sum / n > TEMP_Limit){
                xSemaphoreGive(xAlarmSem);
            }
            sum = n =0 ;
        }
    }
}

void Log(const char *msg){
    xSemaphoreTake(xLogMutex , portMAX_DELAY);
    puts(msg);
    xSemaphoreGive(xLogMutex);
}

void app_main() {
     
    gpio_set_direction(buzzer , GPIO_MODE_OUTPUT);
    xLogMutex = xSemaphoreCreateMutex();
    xAlarmSem = xSemaphoreCreateBinary();
    xSensorQueue = xQueueCreate(5 , sizeof(int));
    
    xTaskCreate(ReadTemp , "Task1" , 3072 , NULL , 3 , NULL);
    xTaskCreate(Alarm , "Task2" , 3072 , NULL , 4 , NULL);
    xTaskCreate(Process , "Task3" , 3072 , NULL , 2 , NULL);

}