#include <services/vsense.h>

#include <state.h>

#include <ports/dev.h>
#include <ports/timer.h>

static float vsense_read()
{
    uint8_t address[2] = {1 << 3, 1 << 3};
    uint8_t data[2];
    // Select ADC channel
    dev_spi_transfer(address, data, sizeof(data));
    // Read ADC value
    dev_spi_transfer(address, data, sizeof(data));
    uint16_t adc = (((uint16_t)(data[0])) & 0b1111) << 8 | data[1];
    return adc * 0.01926f;
}

loop_t loop_vsense;

static void vsense_setup()
{
    dev_spi_enable(true);
    loop_init(&loop_vsense, 100000000); // 100ms
    state->battery_voltage = vsense_read();
}

static void vsense_loop()
{
    // VSense loop
    uint32_t dt_ns = 0;
    if (loop_update(&loop_vsense, &dt_ns))
    {
        // Update vsense value with IIR filter
        state->battery_voltage = state->battery_voltage * 0.99 + vsense_read() * 0.01;
    }
}

em_service_t service_vsense = {
    .state_mask = EM_STATE_ALL,
    .setup = vsense_setup,
    .loop = vsense_loop,
    .teardown = NULL,
};