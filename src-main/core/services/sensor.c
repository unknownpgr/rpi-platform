#include <services/sensor.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <ports/dev.h>
#include <ports/log.h>
#include <ports/timer.h>

#define NUM_SENSORS 16
#define CALIBRATION_FILE "calibration.bin"

#define IR_S03 23
#define IR_S02 24
#define IR_S01 25
#define IR_S00 26
#define IR_SEN 27

static int spi_fd;

typedef struct
{
    uint16_t low[NUM_SENSORS];
    uint16_t high[NUM_SENSORS];
} calibration_t;

typedef union
{
    calibration_t data;
    uint8_t buf[sizeof(calibration_t)];
} calibration_union_t;

void sensor_save_calibration()
{
    calibration_union_t union_data = {0};
    uint8_t buf[sizeof(union_data.buf) + 5] = {0};

    // Set magic number
    buf[0] = 'C';
    buf[1] = 'A';
    buf[2] = 'L';
    buf[3] = 'I';

    // Copy calibration data into buffer
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        union_data.data.low[i] = state->sensor_low[i];
        union_data.data.high[i] = state->sensor_high[i];
    }
    memcpy(buf + 4, union_data.buf, sizeof(union_data.buf));

    // Calculate checksum
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < sizeof(buf) - 1; i++)
    {
        checksum += buf[i];
    }
    buf[sizeof(buf) - 1] = checksum;

    // Write calibration data to file
    FILE *file = fopen(CALIBRATION_FILE, "wb");
    if (file == NULL)
    {
        error("Failed to open calibration file. Calibration not saved.");
        return;
    }
    fwrite(buf, sizeof(buf), 1, file);
    fclose(file);
    print("Calibration saved to %s", CALIBRATION_FILE);
}

void sensor_load_calibration()
{
    // Read calibration data from file
    FILE *file = fopen(CALIBRATION_FILE, "rb");
    if (file == NULL)
    {
        warning("Failed to open calibration file. Not calibrated.");
        return;
    }

    // Read calibration data
    uint8_t buf[sizeof(calibration_union_t) + 5] = {0};

    fread(buf, sizeof(buf), 1, file);
    fclose(file);

    // Validate magic number
    if (buf[0] != 'C' || buf[1] != 'A' || buf[2] != 'L' || buf[3] != 'I')
    {
        error("Invalid magic number. Not calibrated.");
        return;
    }

    // Check checksum
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < sizeof(buf) - 1; i++)
    {
        checksum += buf[i];
    }
    if (checksum != buf[sizeof(buf) - 1])
    {
        error("Invalid checksum. Not calibrated.");
        return;
    }

    // Copy buffer into calibration data
    calibration_union_t union_data = {0};
    memcpy(&union_data.buf, buf + 4, sizeof(union_data.buf));
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        state->sensor_low[i] = union_data.data.low[i];
        state->sensor_high[i] = union_data.data.high[i];
    }

    print("Calibration loaded from %s", CALIBRATION_FILE);
}

void sensor_init()
{
    // Set GPIO mode
    dev_spi_enable(true);

    dev_gpio_set_mode(IR_S03, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_S02, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_S01, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_S00, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_SEN, GPIO_FSEL_OUT);

    sensor_load_calibration();
}

void sensor_select(uint8_t sensor_index)
{
    uint8_t s0 = sensor_index & 0b0001;
    uint8_t s1 = sensor_index & 0b0010;
    uint8_t s2 = sensor_index & 0b0100;
    uint8_t s3 = sensor_index & 0b1000;

    // Select multiplexer channel
    if (s0)
    {
        dev_gpio_set_pin(IR_S00);
    }
    else
    {
        dev_gpio_clear_pin(IR_S00);
    }

    if (s1)
    {
        dev_gpio_set_pin(IR_S01);
    }
    else
    {
        dev_gpio_clear_pin(IR_S01);
    }

    if (s2)
    {
        dev_gpio_set_pin(IR_S02);
    }
    else
    {
        dev_gpio_clear_pin(IR_S02);
    }

    if (s3)
    {
        dev_gpio_set_pin(IR_S03);
    }
    else
    {
        dev_gpio_clear_pin(IR_S03);
    }
}

uint16_t sensor_read_raw(uint8_t sensor_index)
{
    uint8_t tx[] = {0 << 3, 0 << 3};
    uint8_t rx[sizeof(tx)] = {0};

    // Select sensor
    sensor_select(sensor_index);

    // Turn on IR LED
    dev_gpio_set_pin(IR_SEN);

    // Read sensor data
    dev_spi_transfer(tx, rx, sizeof(tx));
    dev_spi_transfer(tx, rx, sizeof(tx));

    // Turn off IR LED
    dev_gpio_clear_pin(IR_SEN);

    // Parse sensor data
    uint16_t data = ((((uint16_t)rx[0]) & 0b1111) << 8) | rx[1];

    // Store sensor data
    return data;
}

void sensor_read_one()
{
    static uint8_t sensor_index = 0;

    // Read raw sensor data
    uint16_t data = sensor_read_raw(sensor_index);
    state->sensor_raw[sensor_index] = data;

    // Normalize ALL sensor data
    for (uint8_t i = 0; i < NUM_SENSORS; i++)
    {
        double data = (double)(state->sensor_raw[i] - state->sensor_low[i]) / (state->sensor_high[i] - state->sensor_low[i]);
        if (data < 0)
        {
            data = 0;
        }
        else if (data > 1)
        {
            data = 1;
        }
        state->sensor_data[i] = data;
    }

    // Increment sensor index
    sensor_index++;
    if (sensor_index >= NUM_SENSORS)
    {
        sensor_index = 0;
    }
}

void sensor_loop()
{
    sensor_read_one();
    switch (state->state)
    {
    case STATE_CALI_LOW:
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            uint16_t data = sensor_read_raw(i);
            if (data > state->sensor_low[i])
            {
                state->sensor_low[i] = data;
            }
        }
        break;
    case STATE_CALI_HIGH:
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            uint16_t data = sensor_read_raw(i);
            if (data > state->sensor_high[i])
            {
                state->sensor_high[i] = data;
            }
        }
        break;
    }
}

void sensor_print_bar(float bar_value)
{
    if (bar_value < 0)
    {
        bar_value = 0;
    }
    else if (bar_value > 1)
    {
        bar_value = 1;
    }

    uint32_t bar_length = (uint32_t)(bar_value * 60);

    char bar[61] = {0};
    for (uint8_t i = 0; i < 60; i++)
    {
        if (i < bar_length)
        {
            bar[i] = '#';
        }
        else
        {
            bar[i] = '_';
        }
    }
    printf("%s", bar);
}

void sensor_test_led()
{
    dev_gpio_set_pin(IR_SEN);

    // Slowly turn on sensors
    while (true)
    {
        for (uint8_t sensor_index = 0; sensor_index < NUM_SENSORS; sensor_index++)
        {
            sensor_select(sensor_index);

            timer_sleep_ns(1e8);

            print("Sensor %d", sensor_index);
        }
    }
}

void sensor_test_raw()
{
    uint16_t sensor_data[NUM_SENSORS];
    while (true)
    {
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            sensor_data[i] = sensor_read_raw(i);
            printf("[%02d] %4d", i, sensor_data[i]);
            sensor_print_bar((float)sensor_data[i] / 4095);
            printf("\n");
        }
        timer_sleep_ns(1e8);
    }
}

void sensor_test_calibration()
{
    uint16_t sensor_data[NUM_SENSORS];
    uint32_t loop_counter = 0;
    uint32_t start_time = timer_get_ns();

    while (true)
    {
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            sensor_data[i] = sensor_read_raw(i);
        }
        loop_counter++;
        uint32_t diff = DIFF(timer_get_ns(), start_time);
        if (diff >= 1e8) // 0.1s
        {
            clear();

            // Calculate loop rate
            float loop_rate = loop_counter / (diff / 1e9);
            print("Loop rate: %f Hz", loop_rate);
            loop_counter = 0;

            // Print sensor data
            printf("     RAW  CALI\n");
            for (uint8_t i = 0; i < 16; i++)
            {
                // Calculate calibrated value
                double calibrated_value = (double)(sensor_data[i] - state->sensor_low[i]) / (state->sensor_high[i] - state->sensor_low[i]);

                // Clamp value into range [0, 1]
                if (calibrated_value < 0)
                {
                    calibrated_value = 0;
                }
                else if (calibrated_value > 1)
                {
                    calibrated_value = 1;
                }

                // Print bar graph
                printf("[%02d] %4d %4.3f |", i, sensor_data[i], calibrated_value);
                sensor_print_bar(calibrated_value);
                printf("\n");
            }
            printf("\n");

            start_time = timer_get_ns();
        }
    }
}
