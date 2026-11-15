/*
 * LCD.c
 *
 * Created: 11/11/2026 11:13:13 AM
 * Author : Sofian.Elaa
 */ 

#include "LCD.h"
#include <util/delay.h>
#include <stdlib.h>

static void LCD_Enable(void)
{
	LCD_PORT_B |= (1 << LCD_EN);
	_delay_us(1);
	LCD_PORT_B &= ~(1 << LCD_EN);
	_delay_us(100);
}

static void LCD_Send4Bits(uint8_t data)
{
	// Clear D4 on PORTB
	LCD_PORT_B &= ~(1 << LCD_D4);
	// Clear D5, D6, D7 on PORTC
	LCD_PORT_C &= ~((1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7));

	// Set D4-D7 based on data bits (D5 is now on PORTC)
	if (data & 0x01) LCD_PORT_B |= (1 << LCD_D4);
	if (data & 0x02) LCD_PORT_C |= (1 << LCD_D5);
	if (data & 0x04) LCD_PORT_C |= (1 << LCD_D6);
	if (data & 0x08) LCD_PORT_C |= (1 << LCD_D7);

	LCD_Enable();
}

static void LCD_Send(uint8_t data)
{
	LCD_Send4Bits(data >> 4);
	LCD_Send4Bits(data & 0x0F);
}

void LCD_Command(uint8_t command)
{
	LCD_PORT_B &= ~(1 << LCD_RS);
	LCD_Send(command);
	_delay_ms(2);
}

void LCD_Char(char data)
{
	LCD_PORT_B |= (1 << LCD_RS);
	LCD_Send(data);
	_delay_us(100);
}

void LCD_String(char *str)
{
	while (*str)
	{
		LCD_Char(*str);
		str++;
	}
}

void LCD_Clear(void)
{
	LCD_Command(0x01);
	_delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
	if (row == 0)
	{
		LCD_Command(0x80 + col);
	}
	else
	{
		LCD_Command(0xC0 + col);
	}
}

void LCD_Init(void)
{
	// Set RS, EN, D4 as OUTPUT on PORTB
	LCD_DDR_B |= (1 << LCD_RS) | (1 << LCD_EN) | (1 << LCD_D4);
	// Set D5, D6, D7 as OUTPUT on PORTC (Analog pins A2, A0, A1)
	LCD_DDR_C |= (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7);

	LCD_PORT_B &= ~((1 << LCD_RS) | (1 << LCD_EN));
	LCD_PORT_C &= ~((1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7));

	_delay_ms(50); // Power-up stabilization delay

	// LCD 4-bit initialization sequence
	LCD_Send4Bits(0x03);
	_delay_ms(5);

	LCD_Send4Bits(0x03);
	_delay_us(150);

	LCD_Send4Bits(0x03);
	_delay_us(150);

	LCD_Send4Bits(0x02); // Set to 4-bit mode
	_delay_ms(1);

	LCD_Command(0x28); // 2 lines, 5x8 matrix
	LCD_Command(0x0C); // Display ON, Cursor OFF
	LCD_Command(0x01); // Clear screen
	_delay_ms(2);
	LCD_Command(0x06); // Increment cursor
}

void LCD_Number(int32_t number)
{
	char buffer[12];
	ltoa(number, buffer, 10);
	LCD_String(buffer);
}