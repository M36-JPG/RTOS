#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

typedef enum
{
    DHT11_OK = 0,
    DHT11_ERROR_TIMEOUT,
    DHT11_ERROR_CHECKSUM
} DHT11_Status;

typedef struct
{
    uint8_t temperature;
    uint8_t humidity;
} DHT11_Data;

void DHT11_Init(void);
DHT11_Status DHT11_Read(DHT11_Data *data);

#endif
