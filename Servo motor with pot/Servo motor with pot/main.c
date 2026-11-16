
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// Servo Initialization
void Servo_Init(void)
{
	DDRB |= (1 << PB1);

	// Fast PWM Mode 14
	// Non-inverting OC1A
	TCCR1A = (1 << COM1A1) | (1 << WGM11);

	// Prescaler = 8
	TCCR1B = (1 << WGM13) |
	(1 << WGM12) |
	(1 << CS11);

	ICR1 = 39999;
}

// ADC Initialization
void ADC_Init(void)
{
	// AVCC as reference voltage
	ADMUX = (1 << REFS0);

	// Enable ADC, Prescaler = 128
	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);
}

// Read ADC Value
uint16_t ADC_Read(uint8_t channel)
{
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

	// Start conversion
	ADCSRA |= (1 << ADSC);

	// Wait until conversion finishes
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

int main(void)
{
	uint16_t pot_value;
	uint16_t servo_value;

	Servo_Init();
	ADC_Init();

	while(1)
	{
		// Read Potentiometer from ADC0
		pot_value = ADC_Read(0);
_delay_ms(20);
		// Map ADC (0-1023) to Servo (2000-4000)
		servo_value = 2000 + ((uint32_t)pot_value * 2000 / 1023);
_delay_ms(20);
		OCR1A = servo_value;

		_delay_ms(20);
	}
}
