#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <dev.h>

// BCM2711
#define GPIO_BASE 0x3F200000
#define PWM0_BASE 0x3F20C000
#define PWM1_BASE 0x3F20C800

int memfd;
uint32_t *gpio_map;
uint32_t *pwm_map[2];

uint32_t *get_map(uint32_t base, uint32_t size)
{
    uint32_t *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, base);
    if (map == MAP_FAILED)
    {
        printf("Could not get mmap\n");
        return NULL;
    }
    return map;
}

bool dev_init()
{
    // Get memory device file descriptor
    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0)
    {
        printf("Could not open mem\n");
        return false;
    }

    // Map GPIO memory
    gpio_map = get_map(GPIO_BASE, 4096);
    if (!gpio_map)
    {
        close(memfd);
        return false;
    }

    // Map PWM memory
    // Note: base address must be aligned to 4KB
    uint32_t *pwm0_base = get_map(PWM0_BASE, 4096);
    if (!pwm0_base)
    {
        close(memfd);
        return false;
    }
    pwm_map[0] = pwm0_base;
    pwm_map[1] = pwm0_base + (PWM1_BASE - PWM0_BASE) / 4;

    return true;
}

void dev_gpio_set_mode(uint32_t pin, uint32_t mode)
{
    // Each GPIO pin has 3 bits for mode
    uint32_t shift = (pin % 10) * 3;
    uint32_t mask = 0b111 << shift;
    gpio_map[pin / 10] = (gpio_map[pin / 10] & ~mask) | ((mode << shift) & mask);
}

void dev_gpio_set_pull(uint32_t pin, uint32_t pull)
{
    // Each GPIO pin has 2 bits for pull
    uint32_t shift = pin % 16;
    uint32_t mask = 0b11 << shift;
    gpio_map[57] = (gpio_map[57] & ~mask) | ((pull << shift) & mask);
}

void dev_gpio_set(uint64_t mask)
{
    gpio_map[7] = mask & 0xFFFFFFFF;
    gpio_map[8] = (mask >> 32) & 0xFFFFFFFF;
}

void dev_gpio_clear(uint64_t mask)
{
    gpio_map[10] = mask & 0xFFFFFFFF;
    gpio_map[11] = (mask >> 32) & 0xFFFFFFFF;
}