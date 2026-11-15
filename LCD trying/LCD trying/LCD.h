#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <stdint.h>


#define F_CPU 16000000UL

// RS, EN, D4 are on PORTB (Digital pins D9 to D11)
#define LCD_PORT_B   PORTB
#define LCD_DDR_B    DDRB

#define LCD_RS       PB1  // D9
#define LCD_EN       PB2  // D10
#define LCD_D4       PB3  // D11

// D5, D6, and D7 are on PORTC (Analog pins A2, A0, and A1)
#define LCD_PORT_C   PORTC
#define LCD_DDR_C    DDRC

#define LCD_D5       PC2  // A2
#define LCD_D6       PC0  // A0
#define LCD_D7       PC1  // A1

void LCD_Init(void);
void LCD_Command(uint8_t command);
void LCD_Char(char data);
void LCD_String(char *str);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Number(int32_t number);

#endif