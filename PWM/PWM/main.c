#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "BIT_MATH.h"

#include <avr/io.h>
#include <avr/interrupt.h>

void Timer0_Init(void)
{
	// Stop Timer0
	// 	TCCR0A = 0;
	// 	TCCR0B = 0;

	// CTC Mode: WGM01 = 1
	TCCR0A |= (1 << WGM01);

	// Compare value for 1 ms
	OCR0A = 249;

	// 	// Enable Output Compare Match A interrupt
	TIMSK0 |= (1 << OCIE0A);

	// Prescaler = 64
	// CS01 = 1, CS00 = 1
	TCCR0B |= (1 << CS01) | (1 << CS00);

	// Enable global interrupts
	sei();
}

ISR(TIMER0_COMPA_vect)
{
	static unsigned char counter=0;
	counter++;
	if(counter == 5)
	CLR_BIT(PORTD,0);
	else if(counter == 10)
	CLR_BIT(PORTD,1);
	else if (counter == 15)
	CLR_BIT(PORTD,2);
	else if(counter == 20)
	{
		SET_BIT(PORTD,0);
		SET_BIT(PORTD,1);
		SET_BIT(PORTD,2);
		counter=0;
	}

}



int main(void)
{
	SET_BIT(DDRD,0);
	SET_BIT(DDRD,1);
	SET_BIT(DDRD,2);

	SET_BIT(PORTD,0);
	SET_BIT(PORTD,1);
	SET_BIT(PORTD,2);

	Timer0_Init();
	
	while (1)
	{
	}
}