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

extern int memfd;
extern uint32_t *gpio_map;
extern uint32_t *pwm_map[2];

bool dev_init();

void dev_gpio_set_mode(uint32_t pin, uint32_t mode);
void dev_gpio_set_pull(uint32_t pin, uint32_t pull);
void dev_gpio_set(uint64_t mask);
void dev_gpio_clear(uint64_t mask);

void dev_pwm_enable(uint32_t index, uint32_t channel);
void dev_pwm_disable(uint32_t index, uint32_t channel);
void dev_pwm_set_range(uint32_t index, uint32_t channel, uint32_t range);
void dev_pwm_set_data(uint32_t index, uint32_t channel, uint32_t data);