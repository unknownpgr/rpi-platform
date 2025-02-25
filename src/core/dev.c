#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <dev.h>
#include <log.h>

// BCM2711
#define GPIO_BASE 0x3F200000
#define PWM0_BASE 0x3F20C000
#define PWM1_BASE 0x3F20C800

// Some MIRACLE register addresses
// Refer to: https://forums.raspberrypi.com/viewtopic.php?t=37770
#define CM_BASE 0x3F101000

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
    - https://pages.hmc.edu/harris/class/e155/09_Ch%2009_online.pdf (Page 31)
    - https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (Page 107)
    - https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf (Page 83)
*/

int memfd;
uint32_t *gpio_map;
uint32_t *pwm_map[2];
uint32_t *cm_map;

void __print_binary(uint32_t data)
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
    printf("%s\n", binary);
}

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

    // Map clock manager memory
    cm_map = get_map(CM_BASE, 4096);
    if (!cm_map)
    {
        close(memfd);
        return false;
    }

    return true;
}

void dev_gpio_set_mode(uint32_t pin, uint32_t mode)
{
    // Each GPIO pin has 3 bits for mode
    uint32_t index = pin / 10;
    uint32_t shift = (pin % 10) * 3;
    uint32_t mask = 0b111 << shift;
    gpio_map[index] = (gpio_map[index] & ~mask) | ((mode << shift) & mask);
}

void dev_gpio_set_pull(uint32_t pin, uint32_t pull)
{
    // Each GPIO pin has 2 bits for pull
    uint32_t shift = pin % 16;
    uint32_t mask = 0b11 << shift;
    gpio_map[37] = (gpio_map[37] & ~mask) | ((pull << shift) & mask);
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

void dev_pwm_enable(uint32_t index, uint32_t channel)
{
#define wait(cond) \
    while (cond)   \
        ;
#define set(reg, bit) (reg |= (1 << (channel * 8 + bit)))
#define cls(reg, bit) (reg &= ~(1 << (channel * 8 + bit)))

    uint32_t *ctl = pwm_map[index] + 0;
    uint32_t *cm_ctl = cm_map + 0xA0 / 4;
    uint32_t *cm_div = cm_map + 0xA4 / 4;

    *ctl = 0; // Disable PWM

    // Configure clock manager
    *cm_ctl = 0x5A000000 | (*cm_ctl & ~(1 << 4));  // Turn off enable flag
    wait(*cm_ctl & (1 << 7));                      // Wait for clock to be turned off
    *cm_div = 0x5A000000 | *cm_ctl | (0x64 << 12); // Configure divider (/100)
    *cm_ctl = 0x5A000000 | *cm_ctl | (1 << 9);     // Set MASH to 1-stage
    *cm_ctl = 0x5A000000 | *cm_ctl | 6;            // Set clock source to PLLD per (500MHz)
    *cm_ctl = 0x5A000000 | *cm_ctl | (1 << 4);     // Turn on enable flag
    wait(!(*cm_ctl & (1 << 7)));                   // Wait for clock to be turned on

    // Therefore it would configure the PWM clock to 5MHz

    uint32_t reg = *ctl;
    set(reg, 0); // Enable PWM channel
    cls(reg, 1); // Set to PWM mode
    cls(reg, 2); // Do not repeat
    cls(reg, 3); // Set state to LOW when there is no data
    cls(reg, 4); // Do not invert the polarity
    cls(reg, 5); // Disable FIFO
    set(reg, 6); // Clear FIFO
    set(reg, 7); // Use M/S mode

    *ctl = reg;

#undef wait
#undef set
#undef cls
}

void dev_pwm_disable(uint32_t index, uint32_t channel)
{
    // Disable PWM channel
    uint32_t *ctl = pwm_map[index] + 0;
    *ctl &= ~(1 << (channel * 8));

    // Set data to 0
    uint32_t *dat = pwm_map[index] + 5 + channel * 4;
    *dat = 0;
}

void dev_pwm_set_range(uint32_t index, uint32_t channel, uint32_t range)
{
    uint32_t *rng = pwm_map[index] + 4 + channel * 4;
    *rng = range;
}

void dev_pwm_set_data(uint32_t index, uint32_t channel, uint32_t data)
{
    uint32_t *dat = pwm_map[index] + 5 + channel * 4;
    *dat = data;
}