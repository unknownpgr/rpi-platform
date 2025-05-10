#include <ports/dev.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdatomic.h>

#if defined(__linux__)
#include <linux/spi/spidev.h>
#include <linux/i2c-dev.h>
#endif

#include <ports/log.h>

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

// SPI pins
#define SPI_CE1 8
#define SPI_MISO 9
#define SPI_SCLK 11
#define SPI_MOSI 10

#define SPI_DEVICE "/dev/spidev0.0" // SPI device
#define SPI_MODE SPI_MODE_0         // SPI mode
#define SPI_BITS 8                  // Bits per word
#define SPI_SPEED 3200000           // Speed in Hz (3.2MHz)
#define SPI_DELAY 0                 // No delay

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

int spi_fd;
struct spi_ioc_transfer spi_transfer;

int i2c_fd;

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

static inline void barrier()
{
    // Add barrier to prevent instruction reordering
    atomic_thread_fence(memory_order_seq_cst);
}

static void sleep_us(uint32_t us)
{
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
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

    // Initialize SPI
#if defined(__linux__)
    {
        spi_fd = open("/dev/spidev0.0", O_RDWR);
        if (spi_fd < 0)
        {
            perror("Failed to open SPI device");
            return false;
        }

        uint8_t mode = SPI_MODE;
        if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0)
        {
            perror("Failed to set SPI mode");
            close(spi_fd);
            return false;
        }

        uint8_t bits = SPI_BITS;
        if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
        {
            perror("Failed to set SPI bits per word");
            close(spi_fd);
            return false;
        }

        uint32_t speed = SPI_SPEED;
        if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
        {
            perror("Failed to set SPI speed");
            close(spi_fd);
            return false;
        }

        spi_transfer.tx_buf = 0;
        spi_transfer.rx_buf = 0;
        spi_transfer.len = 2;
        spi_transfer.speed_hz = SPI_SPEED;
        spi_transfer.delay_usecs = SPI_DELAY;
        spi_transfer.bits_per_word = SPI_BITS;
    }

    // Initialize I2C
    {
        i2c_fd = open("/dev/i2c-1", O_RDWR);
        if (i2c_fd < 0)
        {
            perror("Failed to open I2C device");
            return false;
        }
    }
#endif

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
    uint32_t value = *reg;
    value &= ~mask;
    value |= (mode << shift);
    *reg = value;
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
    barrier();                     // Prevent instruction reordering
    sleep_us(1);                   // Wait for 150 cycles
    *reg_pudclk = 1 << (pin % 32); // Assert clock
    barrier();                     // Prevent instruction reordering
    sleep_us(1);                   // Wait for 150 cycles
    *reg_pud = 0;                  // Remove pull up/down
    barrier();                     // Prevent instruction reordering
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

    // Set data to 0
    *dat = 0;

    if (!enable)
    {
        // Disable PWM channel
        *ctl &= ~(1 << (channel * 8));
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
        barrier();                                   // Prevent instruction reordering
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

// SPI

void dev_spi_enable(bool enable)
{
    if (enable)
    {
        dev_gpio_set_mode(SPI_CE1, GPIO_FSEL_OUT);
        dev_gpio_set_mode(SPI_MISO, GPIO_FSEL_ALT0);
        dev_gpio_set_mode(SPI_MOSI, GPIO_FSEL_ALT0);
        dev_gpio_set_mode(SPI_SCLK, GPIO_FSEL_ALT0);
    }
    else
    {
        dev_gpio_set_mode(SPI_CE1, GPIO_FSEL_IN);
        dev_gpio_set_mode(SPI_MISO, GPIO_FSEL_IN);
        dev_gpio_set_mode(SPI_MOSI, GPIO_FSEL_IN);
        dev_gpio_set_mode(SPI_SCLK, GPIO_FSEL_IN);
    }
}

void dev_spi_transfer(uint8_t *tx, uint8_t *rx, uint32_t len)
{
#if defined(__linux__)
    spi_transfer.tx_buf = (unsigned long)tx;
    spi_transfer.rx_buf = (unsigned long)rx;
    spi_transfer.len = len;
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0)
    {
        perror("SPI transfer failed");
        close(spi_fd);
        return;
    }
#endif
}

// I2C

void dev_i2c_enable(bool enable)
{
    if (enable)
    {
        dev_gpio_set_mode(2, GPIO_FSEL_ALT0);
        dev_gpio_set_mode(3, GPIO_FSEL_ALT0);
    }
    else
    {
        dev_gpio_set_mode(2, GPIO_FSEL_IN);
        dev_gpio_set_mode(3, GPIO_FSEL_IN);
    }
}

bool dev_i2c_read_register(uint8_t addr, uint8_t reg, uint8_t *rx, uint32_t len)
{
#if defined(__linux__)
    if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C slave address");
        return false;
    }

    if (write(i2c_fd, &reg, 1) != 1)
    {
        perror("Failed to write register address to I2C device");
        return false;
    }

    if (read(i2c_fd, rx, len) != len)
    {
        perror("Failed to read from I2C device");
        return false;
    }

    return true;
#endif
}

bool dev_i2c_write_register(uint8_t addr, uint8_t reg, uint8_t *tx, uint32_t len)
{
#if defined(__linux__)
    if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0)
    {
        perror("Failed to set I2C slave address");
        return false;
    }

    uint8_t buffer[len + 1];
    buffer[0] = reg;
    for (int i = 0; i < len; i++)
    {
        buffer[i + 1] = tx[i];
    }

    if (write(i2c_fd, buffer, len + 1) != len + 1)
    {
        perror("Failed to write register address and data to I2C device");
        return false;
    }

    return true;
#endif
}