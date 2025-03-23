#include <vsense-service.h>

#include <dev.h>

void vsense_init()
{
    dev_spi_enable(true);
}

float vsense_read()
{
    uint8_t address[2] = {1 << 3, 1 << 3};
    uint8_t data[2];
    dev_spi_transfer(address, data, sizeof(data));
    uint16_t adc = (((uint16_t)(data[0])) & 0b1111) << 8 | data[1];
    return adc * 0.01926f;
}