#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <stdint.h>

#define LCD_PORT PORTB
#define LCD_DDR  DDRB

#define LCD_RS   PB0
#define LCD_EN   PB2
				  
#define LCD_D4   PB4
#define LCD_D5   PB5
#define LCD_D6   PB6
#define LCD_D7   PB7

void LCD_Init(void);

void LCD_Command(uint8_t command);
void LCD_Char(char data);
void LCD_String(char *str);

void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Number(int32_t number);
#endif