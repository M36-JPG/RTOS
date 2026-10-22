#include<stdint.h>
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"driver/gpio.h"
#include"freertos/semphr.h"
#include"freertos/queue.h"
#include"esp_random.h"

#define buzzer 27
static SemaphoreHandle_t xCarSem;
static SemaphoreHandle_t xAlarmSem;
static QueueHandle_t xCarQueue;

void CarQueue(void *pv){
    
    int max = 0 ;
    
    for(;;){
       if(xSemaphoreTake(xCarSem , pdMS_TO_TICKS(100)) == pdTRUE)
       { // to make sure that the semaphore is work

        max++;
         xQueueSend(xCarQueue , &max , 0);
         printf("Car ordered"); 
        vTaskDelay(pdMS_TO_TICKS(3000)); // wait until I make the order
        printf("Car is out");
        xSemaphoreGive(xCarSem);
       }
      else{
        printf("Car didn't take the order");
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
    }


}

void Alarm(void *pv){
    int var;
    for(;;){
        xQueueReceive(xCarQueue , &var ,pdMS_TO_TICKS(200) );
        if (var >= 3 ){
        gpio_set_level(buzzer , 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(buzzer , 0);
        }
      
    }

}



void app_main() {

    gpio_set_direction(buzzer , GPIO_MODE_OUTPUT);
    xCarSem = xSemaphoreCreateCounting(3 , 3);
    xCarQueue = xQueueCreate(3 , sizeof(int));

    for(int i = 0 ;i <= 6 ; i++){
        xTaskCreate(CarQueue , "Task" , 3072 , NULL , 4 , NULL);
    }
    xTaskCreate(Alarm , "Task3" , 3072 , NULL , 2 , NULL);
}