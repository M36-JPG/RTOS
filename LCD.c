#include "LCD.h"
#include <util/delay.h>
#include <stdlib.h>


static void LCD_Enable(void)
{
    LCD_PORT |= (1 << LCD_EN);

    _delay_us(1);

    LCD_PORT &= ~(1 << LCD_EN);

    _delay_us(100);
}


static void LCD_Send4Bits(uint8_t data)
{
    /* Clear D4-D7 */

    LCD_PORT &= ~((1 << LCD_D4) |
                  (1 << LCD_D5) |
                  (1 << LCD_D6) |
                  (1 << LCD_D7));


    /* D4 */

    if (data & 0x01)
        LCD_PORT |= (1 << LCD_D4);


    /* D5 */

    if (data & 0x02)
        LCD_PORT |= (1 << LCD_D5);


    /* D6 */

    if (data & 0x04)
        LCD_PORT |= (1 << LCD_D6);


    /* D7 */

    if (data & 0x08)
        LCD_PORT |= (1 << LCD_D7);


    LCD_Enable();
}


static void LCD_Send(uint8_t data)
{
    LCD_Send4Bits(data >> 4);

    LCD_Send4Bits(data & 0x0F);
}


void LCD_Command(uint8_t command)
{
    /* RS = 0 */

    LCD_PORT &= ~(1 << LCD_RS);

    LCD_Send(command);

    _delay_ms(2);
}


void LCD_Char(char data)
{
    /* RS = 1 */

    LCD_PORT |= (1 << LCD_RS);

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
    /* RS, EN, D4-D7 OUTPUT */

    LCD_DDR |= (1 << LCD_RS) |
               (1 << LCD_EN) |
               (1 << LCD_D4) |
               (1 << LCD_D5) |
               (1 << LCD_D6) |
               (1 << LCD_D7);


    /* Initially LOW */

    LCD_PORT &= ~((1 << LCD_RS) |
                  (1 << LCD_EN));


    _delay_ms(20);


    /*
       LCD Initialization
       ------------------
       Send 0x03 three times
    */

    LCD_Send4Bits(0x03);

    _delay_ms(5);


    LCD_Send4Bits(0x03);

    _delay_us(150);


    LCD_Send4Bits(0x03);

    _delay_us(150);


    /*
       Change to 4-bit mode
    */

    LCD_Send4Bits(0x02);

    _delay_ms(1);


    /*
       Function Set
       4-bit
       2 lines
       5x8 font
    */

    LCD_Command(0x28);


    /*
       Display ON
       Cursor OFF
       Blink OFF
    */

    LCD_Command(0x0C);


    /*
       Clear Display
    */

    LCD_Command(0x01);

    _delay_ms(2);


    /*
       Entry Mode
       Cursor moves right
    */

    LCD_Command(0x06);
}
void LCD_Number(int32_t number)
{
	char buffer[12];

	ltoa(number, buffer, 10);

	LCD_String(buffer);
}