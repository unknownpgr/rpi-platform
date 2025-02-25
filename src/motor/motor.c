#include <stdio.h>
#include <unistd.h>
#include <motor.h>
#include <dev.h>

#define GPIO_PIN_EN 18
#define GPIO_PIN_L_DIR 16
#define GPIO_PIN_R_DIR 17
#define GPIO_PIN_L_PWM 12 // PWM0_0
#define GPIO_PIN_R_PWM 13 // PWM0_1

bool motor_init()
{
    dev_gpio_set_mode(GPIO_PIN_EN, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_L_DIR, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_R_DIR, GPIO_FSEL_OUT);

    dev_gpio_set_pull(GPIO_PIN_EN, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_L_DIR, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_R_DIR, GPIO_PUD_DOWN);

    dev_gpio_set_mode(GPIO_PIN_L_PWM, GPIO_FSEL_ALT0);
    dev_gpio_set_mode(GPIO_PIN_R_PWM, GPIO_FSEL_ALT0);

    dev_pwm_set_range(0, 0, 1000);
    dev_pwm_set_range(0, 1, 1000);
    // PWM clock is 5MHz, so the frequency is 5kHz.

    return true;
}

void motor_enable(bool enable)
{
    if (enable)
    {
        dev_gpio_set(1 << GPIO_PIN_EN);
        dev_pwm_enable(0, 0);
        dev_pwm_enable(0, 1);
    }
    else
    {
        dev_gpio_clear(1 << GPIO_PIN_EN);
        dev_pwm_disable(0, 0);
        dev_pwm_disable(0, 1);
    }
}

void motor_set_velocity(float vL, float vR)
{
    if (vR > 0)
    {
        dev_gpio_set(1 << GPIO_PIN_R_DIR);
        uint32_t data = (uint32_t)((1 - vR) * 1000);
        dev_pwm_set_data(0, 0, data);
        dev_pwm_set_data(0, 1, data);
    }
    else
    {
        dev_gpio_clear(1 << GPIO_PIN_R_DIR);
        uint32_t data = (uint32_t)(vR * -1000);
        dev_pwm_set_data(0, 0, data);
        dev_pwm_set_data(0, 1, data);
    }
}