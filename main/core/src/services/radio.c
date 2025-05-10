#include <services/radio.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <ports/timer.h>
#include <ports/dev.h>
#include <ports/log.h>

#define BCM2835_PERI_BASE 0x3F000000              // Raspberry Pi 3
#define CLOCK_BASE (BCM2835_PERI_BASE + 0x101000) // Clock register address
#define GPIO_BASE (BCM2835_PERI_BASE + 0x200000)  // GPIO register address

void radio_play(char *music_file_path)
{
    // Load music file
    FILE *fp = fopen(music_file_path, "rb");
    if (fp == NULL)
    {
        printf("File open error\n");
        return;
    }
    fseek(fp, 0, SEEK_END);
    uint32_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *music_data = (uint8_t *)malloc(file_size);
    fread(music_data, 1, file_size, fp);
    fclose(fp);

    // Set GPIO 4 as ALT0 mode
    dev_gpio_set_mode(4, GPIO_FSEL_ALT0);
    print("GPIO 4 set as ALT0 mode");

    // Enable PLLD
    dev_gpclk_enable(0, true);
    print("Clock started.");

    uint32_t i = 0;
    uint32_t last_time = timer_get_ns();

    while (true)
    {
        uint32_t current_time = timer_get_ns();
        uint32_t elapsed_time = DIFF(current_time, last_time);
        if (elapsed_time >= 22676)
        {
            last_time = current_time;
            float current_value = ((music_data[i]) / 255.f) - 0.5f; // Convert uint8_t to float
            i++;

            double deviation = (current_value) * 750000; // 25kHz modulation
            double freq = 100e6 + deviation;             // 100MHz carrier frequency
            double divisor = 500e6 / freq;               // 500MHz / freq

            uint32_t divisor_integer = divisor * (1 << 20);
            uint32_t part_integer = divisor_integer >> 20;
            uint32_t part_fraction = divisor_integer & 0xFFFFF;

            dev_gpclk_set_divisor(0, part_integer, part_fraction);

            if (i >= file_size)
            {
                break;
            }
        }
    }
}