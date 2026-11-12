//#include "config.h"
#include "dht11.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#define  DHT_PINREG   PIND
#define  DHT_PIN      PD2
#define   DHT_DDR     DDRD
#define  DHT_PORT      PORTD
// Wait for the DATA line to reach level (0 = LOW, 1 = HIGH).
// The timeout is a coarse software timeout; pulse timing should be
// calibrated for the selected compiler and optimization settings.
static uint8_t DHT_WaitForLevel(uint8_t level)
{
    uint16_t timeout = 0;

    while ((((DHT_PINREG & (1U << DHT_PIN)) != 0U) ? 1U : 0U) != level)
    {
        if (++timeout >= 120U)
            return 0U;

        _delay_us(1);
    }

    return 1U;
}

void DHT11_Init(void)
{
    // Release DATA and enable the AVR's internal pull-up.
    // An external 4.7k-10k pull-up to +5V is recommended.
    DHT_PORT |= (1U << DHT_PIN);
    DHT_DDR  &= ~(1U << DHT_PIN);
}

static void DHT_Start(void)
{
    // Drive the bus LOW for at least 18 ms.
    DHT_PORT &= ~(1U << DHT_PIN);
    DHT_DDR  |=  (1U << DHT_PIN);
    _delay_ms(20);

    // Release the bus (input with pull-up).
    DHT_DDR  &= ~(1U << DHT_PIN);
    DHT_PORT |=  (1U << DHT_PIN);
    _delay_us(30);
}

static uint8_t DHT_ReadBit(uint8_t *bit)
{
    uint16_t high_time = 0;

    // Each data bit begins with ~50 us LOW, followed by HIGH.
    if (!DHT_WaitForLevel(1U))
        return 0U;

    // Measure the HIGH pulse until the sensor pulls DATA LOW.
    while (DHT_PINREG & (1U << DHT_PIN))
    {
        if (++high_time >= 120U)
            return 0U;

        _delay_us(1);
    }

    // Typical HIGH: ~26-28 us for 0, ~70 us for 1.
    // This software-loop threshold is approximate.
    *bit = (high_time > 40U) ? 1U : 0U;
    return 1U;
}

static uint8_t DHT_ReadByte(uint8_t *value)
{
    uint8_t i;
    uint8_t bit;

    *value = 0U;

    for (i = 0U; i < 8U; i++)
    {
        if (!DHT_ReadBit(&bit))
            return 0U;

        *value = (uint8_t)((*value << 1) | bit);
    }

    return 1U;
}

DHT11_Status DHT11_Read(DHT11_Data *data)
{
    uint8_t bytes[5];
    uint8_t i;
    uint8_t old_sreg;

    if (data == 0)
        return DHT11_ERROR_TIMEOUT;

    old_sreg = SREG;
    cli();

    DHT_Start();

    // Sensor response: LOW ~80 us, then HIGH ~80 us.
    if (!DHT_WaitForLevel(0U))
        goto timeout_error;

    if (!DHT_WaitForLevel(1U))
        goto timeout_error;

    if (!DHT_WaitForLevel(0U))
        goto timeout_error;

    // Receive humidity integer/decimal, temperature integer/decimal,
    // and checksum: 5 bytes = 40 bits.
    for (i = 0U; i < 5U; i++)
    {
        if (!DHT_ReadByte(&bytes[i]))
            goto timeout_error;
    }

    SREG = old_sreg;

    if ((uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]) != bytes[4])
        return DHT11_ERROR_CHECKSUM;

    // This example displays integer values only.
    data->humidity    = bytes[0];
    data->temperature = bytes[2];

    return DHT11_OK;

timeout_error:
    SREG = old_sreg;
    return DHT11_ERROR_TIMEOUT;
}
