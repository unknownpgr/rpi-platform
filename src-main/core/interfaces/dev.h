#pragma once

#include <stdint.h>
#include <stdbool.h>

#define GPIO_FSEL_IN 0
#define GPIO_FSEL_OUT 1
#define GPIO_FSEL_ALT0 4
#define GPIO_FSEL_ALT1 5
#define GPIO_FSEL_ALT2 6
#define GPIO_FSEL_ALT3 7
#define GPIO_FSEL_ALT4 3
#define GPIO_FSEL_ALT5 2

#define GPIO_PUD_OFF 0
#define GPIO_PUD_DOWN 1
#define GPIO_PUD_UP 2

bool dev_init();

void dev_gpio_set_mode(uint32_t pin, uint32_t mode);
void dev_gpio_set_pull(uint32_t pin, uint32_t pull);
void dev_gpio_set_mask(uint64_t mask);
void dev_gpio_set_pin(uint32_t pin);
void dev_gpio_clear_mask(uint64_t mask);
void dev_gpio_clear_pin(uint32_t pin);
bool dev_gpio_get_pin(uint32_t pin);

void dev_pwm_enable(uint32_t channel, bool enable);
void dev_pwm_set_range(uint32_t channel, uint32_t range);
void dev_pwm_set_data(uint32_t channel, uint32_t data);

void dev_gpclk_enable(uint32_t index, bool enable);
void dev_gpclk_set_divisor(uint32_t index, uint32_t integer, uint32_t fraction);

void dev_spi_enable(bool enable);
void dev_spi_transfer(uint8_t *tx, uint8_t *rx, uint32_t len);

void dev_i2c_enable(bool enable);
bool dev_i2c_read_register(uint8_t addr, uint8_t reg, uint8_t *data, uint32_t len);
bool dev_i2c_write_register(uint8_t addr, uint8_t reg, uint8_t *data, uint32_t len);