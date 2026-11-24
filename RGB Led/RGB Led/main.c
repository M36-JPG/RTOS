#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

// توصيل الأرجل التلاتة على Port D (مع مقاومة 220Ω - 330Ω على كل رجل)
#define PIN_1 PD5
#define PIN_2 PD6
#define PIN_3 PD7

int main(void) {
	
	DDRD |= (1 << PIN_1) | (1 << PIN_2) | (1 << PIN_3);

	while (1) {
		PORTD |= (1 << PIN_1);
		PORTD &= ~((1 << PIN_2) | (1 << PIN_3));
		_delay_ms(600);

		PORTD |= (1 << PIN_2);
		PORTD &= ~((1 << PIN_1) | (1 << PIN_3));
		_delay_ms(600);

		PORTD |= (1 << PIN_3);
		PORTD &= ~((1 << PIN_1) | (1 << PIN_2));
		_delay_ms(600);

		PORTD |= (1 << PIN_1) | (1 << PIN_2) | (1 << PIN_3);
		_delay_ms(600);
	}
}