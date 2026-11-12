/*
 * sensor DHD11.c
 *
 * Created: 11/11/2026 10:13:28 AM
 * Author : Sama.Adel
 */ 

// #include <avr/io.h>
// #include "LCD.h"
// #include "STD_TYPES.h"
// #include "BIT_MATH.h"
// 
// int main(void)
// {
//     /* Replace with your application code */
// 		LCD_Init();
// 		timer_init();
// 		
// 		//PROJECT 3
// 		 	uint8_t data[5];
// 		 	uint8_t humidity;
// 		 	uint8_t temp;
// 	     
//     while (1) 
//     {
// 		//PROJECT 3
// 		
// 		 		if(read (data)){
// 		 			humidity = data[0];
// 		 			temp = data[2];
// 		
// 					LCD_String("humidity = ")
// 		 			LCD_Number(humidity);
// 		 			LCD_SetCursor(1 ,0);
// 		 			LCD_String("Temp = ")
// 		 			LCD_Number(temp);
// 					_delay_ms(100);
// 	  			    LCD_Clear();
// 		
// 		
// 		 		}
// 		
// 		 		else{
// 		
// 		 	    }
// }

//#include "config.h"
#include <util/delay.h>
#include "dht11.h"
#include "LCD.h"

static void LCD_PrintNumber(uint8_t number)
{
	if (number >= 100U)
	LCD_Char((char)('0' + number / 100U));

	if (number >= 10U)
	LCD_Char((char)('0' + (number / 10U) % 10U));

	LCD_Char((char)('0' + number % 10U));
}

int main(void)
{
	DHT11_Data sensor;
	DHT11_Status status;

	LCD_Init();
	DHT11_Init();
	LCD_String("Hello Sama");

	LCD_SetCursor(0U, 0U);
	LCD_String("DHT11 Monitor");
	LCD_SetCursor(1U, 0U);
	LCD_String("Starting...");
	_delay_ms(2000);

	while (1)
	{
		status = DHT11_Read(&sensor);
		LCD_Clear();

		if (status == DHT11_OK)
		{
			LCD_SetCursor(0U, 0U);
			LCD_String("Temp: ");
			LCD_PrintNumber(sensor.temperature);
			LCD_String(" C");

			LCD_SetCursor(1U, 0U);
			LCD_String("Hum : ");
			LCD_PrintNumber(sensor.humidity);
			LCD_String(" %");
		}
		else
		{
			LCD_SetCursor(0U, 0U);
			LCD_String("Sensor Error");
			LCD_SetCursor(1U, 0U);

			if (status == DHT11_ERROR_CHECKSUM)
			LCD_String("Checksum Error");
			else
			LCD_String("Timeout Error");
		}

		// DHT11 should not be polled more frequently than about once/sec.
		_delay_ms(2000);
	}
}


