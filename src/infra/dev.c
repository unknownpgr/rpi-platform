#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <dev.h>
#include <log.h>

#define PAGE_SIZE 4096

// BCM2711
#define PERI_BASE 0x3F000000

// Note: base address must be aligned to page size (4kB)
#define GPIO_BASE (PERI_BASE + 0x200000)
#define GPIO_FSEL_OFFSET 0x00
#define GPIO_SET_OFFSET 0x1C
#define GPIO_CLR_OFFSET 0x28
#define GPIO_LEV_OFFSET 0x34
#define GPIO_PUD_OFFSET 0x94
#define GPIO_PUDCLK_OFFSET 0x98

#define PWM_BASE (PERI_BASE + 0x20C000)
#define PWM0_OFFSET 0x000
#define PWM1_OFFSET 0x800

// Note: PWM registers(0x3F1010A0, 0x3F1010A4) are not properly documented.
#define CM_BASE 0x3F101000
#define GPCLK_OFFSET 0x70
#define CM_PWMCTL_OFFSET 0xA0
#define CM_PWMDIV_OFFSET 0xA4

// Utility macros
#define WAIT(cond) \
    while (cond)   \
        ;
#define SET(reg, bit, value) ((reg) = ((reg) & ~(1 << (bit))) | (value << (bit)))

/*
Clock Manager Register (CM_CTL) Layout
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| Bit Number     | Field Name                                               | Description                                                               | Read/Write | Reset |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 31-24          | PASSWD                                                   | Clock Manager password “5a”                                               | W          | 0     |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 23-11          | -                                                        | Unused                                                                    | R          | 0     |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 10-9           | MASH                                                     | MASH control                                                              | R/W        | 0     |
|                |                                                          | 0 = integer division                                                      |            |       |
|                |                                                          | 1 = 1-stage MASH (equivalent to non-MASH dividers)                        |            |       |
|                |                                                          | 2 = 2-stage MASH                                                          |            |       |
|                |                                                          | 3 = 3-stage MASH                                                          |            |       |
|                |                                                          | To avoid lock-ups and glitches do not change this control while BUSY=1    |            |       |
|                |                                                          | and do not change this control at the same time as asserting ENAB.        |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 8              | FLIP                                                     | Invert the clock generator output                                         | R/W        | 0     |
|                |                                                          | This is intended for use in test/debug only. Switching this control       |            |       |
|                |                                                          | will generate an edge on the clock generator output. To avoid output      |            |       |
|                |                                                          | glitches do not switch this control while BUSY=1.                         |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 7              | BUSY                                                     | Clock generator is running                                                | R          | 0     |
|                |                                                          | Indicates the clock generator is running. To avoid glitches and lock-ups, |            |       |
|                |                                                          | clock sources and setups must not be changed while this flag is set.      |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 6              | -                                                        | Unused                                                                    | R          | 0     |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 5              | KILL                                                     | Kill the clock generator                                                  | R/W        | 0     |
|                |                                                          | 0 = no action                                                             |            |       |
|                |                                                          | 1 = stop and reset the clock generator                                    |            |       |
|                |                                                          | This is intended for test/debug only. Using this control may cause a      |            |       |
|                |                                                          | glitch on the clock generator output.                                     |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 4              | ENAB                                                     | Enable the clock generator                                                | R/W        | 0     |
|                |                                                          | This requests the clock to start or stop without glitches. The output     |            |       |
|                |                                                          | clock will not stop immediately because the cycle must be allowed to      |            |       |
|                |                                                          | complete to avoid glitches. The BUSY flag will go low when the final      |            |       |
|                |                                                          | cycle is completed.                                                       |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 3-0            | SRC                                                      | Clock source                                                              | R/W        | 0     |
|                |                                                          | 0 = GND                                                                   |            |       |
|                |                                                          | 1 = oscillator                                                            |            |       |
|                |                                                          | 2 = testdebug0                                                            |            |       |
|                |                                                          | 3 = testdebug1                                                            |            |       |
|                |                                                          | 4 = PLLA per                                                              |            |       |
|                |                                                          | 5 = PLLC per                                                              |            |       |
|                |                                                          | 6 = PLLD per                                                              |            |       |
|                |                                                          | 7 = HDMI auxiliary                                                        |            |       |
|                |                                                          | 8-15 = GND                                                                |            |       |
|                |                                                          | To avoid lock-ups and glitches do not change this control while BUSY=1    |            |       |
|                |                                                          | and do not change this control at the same time as asserting ENAB.        |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+


Clock Manager Register (CM_DIV) Layout
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| Bit Number     | Field Name                                               | Description                                                               | Read/Write | Reset |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 31-24          | PASSWD                                                   | Clock Manager password “5a”                                               | W          | 0     |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 23-12          | DIVI                                                     | Integer part of divisor                                                   | R/W        | 0     |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
|                |                                                          | This value has a minimum limit determined by the                          |            |       |
|                |                                                          | MASH setting. See text for details. To avoid lock-ups                     |            |       |
|                |                                                          | and glitches do not change this control while BUSY=1.                     |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+
| 11-0           | DIVF                                                     | Fractional part of divisor                                                | R/W        | 0     |
|                |                                                          | To avoid lock-ups and glitches do not change this control                 |            |       |
|                |                                                          | while BUSY=1.                                                             |            |       |
+----------------+----------------------------------------------------------+---------------------------------------------------------------------------+------------+-------+


References:
    - https://forums.raspberrypi.com/viewtopic.php?t=37770
    - https://pages.hmc.edu/harris/class/e155/09_Ch%2009_online.pdf (Page 31)
    - https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (Page 107)
    - https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf (Page 83)
*/

static uint32_t *gpio_base;
static uint32_t *pwm_base;
static uint32_t *cm_base;

static void print_binary(uint32_t data)
{
    char binary[100];
    int index = 0;
    for (int i = 0; i < 32; i++)
    {
        binary[index++] = (data & 0x80000000) ? '1' : '0';
        data <<= 1;
        if (i % 4 == 3)
        {
            binary[index++] = ' ';
        }
    }
    binary[index] = '\0';
    printf("%s", binary);
}

static uint32_t *get_map(int memfd, uint32_t base, uint32_t size)
{
    uint32_t *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, base);
    if (map == MAP_FAILED)
    {
        return NULL;
    }
    return map;
}

bool dev_init()
{
    // Get memory device file descriptor
    int memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0)
    {
        return false;
    }

    // Map GPIO memory
    gpio_base = get_map(memfd, GPIO_BASE, PAGE_SIZE);
    if (gpio_base == NULL)
    {
        close(memfd);
        return false;
    }

    // Map PWM memory
    uint32_t *pwm0_base = get_map(memfd, PWM_BASE, PAGE_SIZE);
    if (pwm0_base == NULL)
    {
        close(memfd);
        return false;
    }
    pwm_base = pwm0_base + PWM0_OFFSET / 4;

    // Map clock manager memory
    cm_base = get_map(memfd, CM_BASE, PAGE_SIZE);
    if (cm_base == NULL)
    {
        close(memfd);
        return false;
    }

    close(memfd);

    return true;
}

// GPIO

void dev_gpio_set_mode(uint32_t pin, uint32_t mode)
{
    // Each GPIO pin has 3 bits for mode
    uint32_t index = pin / 10;
    uint32_t shift = (pin % 10) * 3;
    uint32_t mask = 0b111 << shift;
    uint32_t *reg = gpio_base + GPIO_FSEL_OFFSET / 4 + index;
    *reg = (*reg & ~mask) | ((mode << shift) & mask);
}

void dev_gpio_set_pull(uint32_t pin, uint32_t pull)
{
    // Each GPIO pin has 2 bits for pull
    uint32_t shift = pin % 16;
    uint32_t mask = 0b11 << shift;

    uint32_t *reg_pud = gpio_base + GPIO_PUD_OFFSET / 4;
    uint32_t *reg_pudclk = gpio_base + GPIO_PUDCLK_OFFSET / 4 + pin / 32;

    // Refer to the BCM2835 datasheet, page 101
    *reg_pud = pull;               // Set pull up/down
    usleep(1);                     // Wait for 150 cycles
    *reg_pudclk = 1 << (pin % 32); // Assert clock
    usleep(1);                     // Wait for 150 cycles
    *reg_pud = 0;                  // Remove pull up/down
    *reg_pudclk = 0;               // Remove clock
}

void dev_gpio_set_mask(uint64_t mask)
{
    gpio_base[7] = mask & 0xFFFFFFFF;
    gpio_base[8] = (mask >> 32) & 0xFFFFFFFF;
}

void dev_gpio_set_pin(uint32_t pin)
{
    uint32_t index = pin / 32;
    uint32_t shift = pin % 32;
    uint32_t mask = 1 << shift;
    gpio_base[7 + index] = mask;
}

void dev_gpio_clear_mask(uint64_t mask)
{
    gpio_base[10] = mask & 0xFFFFFFFF;
    gpio_base[11] = (mask >> 32) & 0xFFFFFFFF;
}

void dev_gpio_clear_pin(uint32_t pin)
{
    uint32_t index = pin / 32;
    uint32_t shift = pin % 32;
    uint32_t mask = 1 << shift;
    gpio_base[10 + index] = mask;
}

bool dev_gpio_get_pin(uint32_t pin)
{
    uint32_t index = pin / 32;
    uint32_t shift = pin % 32;
    uint32_t mask = 1 << shift;
    uint32_t value = (gpio_base[13 + index] & mask) >> shift;
    return value;
}

// PWM

void dev_pwm_enable(uint32_t channel, bool enable)
{
#define PASSWORD 0x5A000000

    uint32_t *ctl = pwm_base;
    uint32_t *dat = pwm_base + 5 + channel * 4;

    if (!enable)
    {
        // Disable PWM channel
        *ctl &= ~(1 << (channel * 8));

        // Set data to 0
        *dat = 0;
        return;
    }

    // Configure PWM clock
    {
        uint32_t *cm_ctl = cm_base + CM_PWMCTL_OFFSET / 4;
        uint32_t *cm_div = cm_base + CM_PWMDIV_OFFSET / 4;

        *cm_ctl = PASSWORD | (*cm_ctl & ~(1 << 4));  // Turn off enable flag
        WAIT(*cm_ctl & (1 << 7));                    // Wait for clock to be turned off
        *cm_div = PASSWORD | *cm_div | (0x19 << 12); // Configure divider (/25)
        *cm_ctl = PASSWORD | *cm_ctl & ~(0b11 << 9); // Set MASH to 0
        *cm_ctl = PASSWORD | *cm_ctl | 6;            // Set clock source to PLLD per (500MHz)
        *cm_ctl = PASSWORD | *cm_ctl | (1 << 4);     // Turn on enable flag
        WAIT(!(*cm_ctl & (1 << 7)));                 // Wait for clock to be turned on

        // Therefore it would configure the PWM clock to 500MHz / 25 = 20MHz
    }

    // Configure PWM channel
    {
        uint32_t reg = *ctl;
        SET(reg, 0 + channel * 8, 1); // Enable PWM channel
        SET(reg, 1 + channel * 8, 0); // Set to PWM mode
        SET(reg, 2 + channel * 8, 0); // Do not repeat
        SET(reg, 3 + channel * 8, 0); // Set state to LOW when there is no data
        SET(reg, 4 + channel * 8, 0); // Do not invert the polarity
        SET(reg, 5 + channel * 8, 0); // Disable FIFO
        SET(reg, 6, 1);               // Clear FIFO
        SET(reg, 7 + channel * 8, 1); // Use M/S mode
        *ctl = reg;
    }

#undef PASSWORD
}

void dev_pwm_set_range(uint32_t channel, uint32_t range)
{
    uint32_t *reg = pwm_base + 4 + channel * 4;
    *reg = range;
}

void dev_pwm_set_data(uint32_t channel, uint32_t data)
{
    uint32_t *reg = pwm_base + 5 + channel * 4;
    *reg = data;
}

// GPCLK

void dev_gpclk_enable(uint32_t index, bool enable)
{
    uint32_t *cm_ctl = cm_base + 0x70 / 4 + index * 0x08 / 4;
    uint32_t *cm_div = cm_base + 0x74 / 4 + index * 0x08 / 4;

    if (!enable)
    {
        *cm_ctl = 0x5A000000 | *cm_ctl & ~(1 << 4); // Disable flag
        while (*cm_ctl & (1 << 7))
            ;
        return;
    }

    *cm_ctl = 0x5A000000 | *cm_ctl & ~(1 << 4); // Disable
    while (*cm_ctl & (1 << 7))
        ;
    *cm_div = 0x5A000000 | 0x1 << 12;          // Divisor 1
    *cm_ctl = 0x5A000000 | 0x06;               // Clock source: PLLD
    *cm_ctl = 0x5A000000 | *cm_ctl | (1 << 4); // Enable
    while (!(*cm_ctl & (1 << 7)))
        ;
}

void dev_gpclk_set_divisor(uint32_t index, uint32_t integer, uint32_t fraction)
{
    uint32_t *cm_ctl = cm_base + 0x70 / 4 + index * 0x08 / 4;
    uint32_t *cm_div = cm_base + 0x74 / 4 + index * 0x08 / 4;

    *cm_ctl = 0x5A000000 | *cm_ctl & ~(1 << 4); // Disable
    while (*cm_ctl & (1 << 7))
        ;
    *cm_div = 0x5A000000 | (integer & 0xFFF) << 12 | (fraction & 0xFFF);
    *cm_ctl = 0x5A000000 | *cm_ctl | (1 << 4); // Enable
    while (!(*cm_ctl & (1 << 7)))
        ;
}