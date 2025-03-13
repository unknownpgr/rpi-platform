#include <sensor-service.h>

#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <dev.h>
#include <log.h>
#include <timer.h>

#define NUM_SENSORS 16

#define SPI_CE1 8
#define SPI_MISO 9
#define SPI_SCLK 11
#define SPI_MOSI 10

#define IR_S03 23
#define IR_S02 24
#define IR_S01 25
#define IR_S00 26
#define IR_SEN 27

#define SPI_DEVICE "/dev/spidev0.0" // SPI device
#define SPI_MODE SPI_MODE_0         // SPI mode
#define SPI_BITS 8                  // Bits per word
#define SPI_SPEED 3200000           // Speed in Hz (3.2MHz)
#define SPI_DELAY 0                 // No delay

static int spi_fd;
static uint16_t black_max[NUM_SENSORS] = {0};
static uint16_t white_max[NUM_SENSORS] = {0};

void sensor_init()
{
    // Set GPIO mode
    dev_gpio_set_mode(SPI_SCLK, GPIO_FSEL_ALT0);
    dev_gpio_set_mode(SPI_MISO, GPIO_FSEL_ALT0);
    dev_gpio_set_mode(SPI_MOSI, GPIO_FSEL_ALT0);
    dev_gpio_set_mode(SPI_CE1, GPIO_FSEL_OUT);

    dev_gpio_set_mode(IR_S03, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_S02, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_S01, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_S00, GPIO_FSEL_OUT);
    dev_gpio_set_mode(IR_SEN, GPIO_FSEL_OUT);

    // SPI configuration
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0)
    {
        perror("Failed to open SPI device");
        return;
    }

    // Set SPI mode
    uint8_t mode = SPI_MODE;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1)
    {
        perror("Failed to set SPI mode");
        close(spi_fd);
        return;
    }

    // Set bits per word
    uint8_t bits = SPI_BITS;
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1)
    {
        perror("Failed to set SPI bits per word");
        close(spi_fd);
        return;
    }

    // Set SPI speed
    uint32_t speed = SPI_SPEED;
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1)
    {
        perror("Failed to set SPI speed");
        close(spi_fd);
        return;
    }
}

void sensor_read(uint16_t *sensor_data)
{
    uint8_t tx[] = {0 << 3, 0 << 3}; // Example data to send
    uint8_t rx[sizeof(tx)] = {0};    // Receive buffer

    struct spi_ioc_transfer spi_transfer = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = sizeof(tx),
        .delay_usecs = SPI_DELAY,
        .speed_hz = SPI_SPEED,
        .bits_per_word = SPI_BITS};

    for (uint8_t sensor_index = 0; sensor_index < NUM_SENSORS; sensor_index++)
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

        // Turn on IR LED
        dev_gpio_set_pin(IR_SEN);

        // Read sensor data
        if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0)
        {
            perror("SPI transfer failed");
            close(spi_fd);
            return;
        }

        // Turn off IR LED
        dev_gpio_clear_pin(IR_SEN);

        // Parse sensor data
        uint16_t data = ((((uint16_t)rx[0]) & 0b1111) << 8) | rx[1];

        // Store sensor data
        sensor_data[sensor_index] = data;
    }
}

void sensor_calibrate()
{
    // Initialize black / white max array
    for (uint8_t i = 0; i < NUM_SENSORS; i++)
    {
        black_max[i] = 0;
        white_max[i] = 0;
    }

    // Read black max
    uint32_t start_time = timer_get_s();
    uint16_t sensor_data[NUM_SENSORS];
    while ((timer_get_s() - start_time) < 4)
    {
        clear();
        print("Collecting black max data...");
        sensor_read(sensor_data);
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            if (sensor_data[i] > black_max[i])
            {
                black_max[i] = sensor_data[i];
            }
        }
    }

    // Read white max
    start_time = timer_get_s();
    while ((timer_get_s() - start_time) < 4)
    {
        clear();
        print("Collecting white max data...");
        sensor_read(sensor_data);
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            if (sensor_data[i] > white_max[i])
            {
                white_max[i] = sensor_data[i];
            }
        }
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
        sensor_read(sensor_data);
        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
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
        sensor_read(sensor_data);
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
                float calibrated_value = (float)(sensor_data[i] - black_max[i]) / (white_max[i] - black_max[i]);

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
