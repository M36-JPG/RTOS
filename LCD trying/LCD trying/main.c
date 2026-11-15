#include <avr/io.h>
#include "LCD.h"
//#include "ultrasonic.h"
#include <util/delay.h>
#include "STD_TYPES.h"
#include "BIT_MATH.h"

int main(void)
{


	LCD_Init();
	LCD_SetCursor(0,0);
	LCD_String("Mariem & Sama");

}
