#include <stdio.h>
#include <unistd.h>
#include <motor.h>
#include <dev.h>

#define GPIO_PIN_EN 18
#define GPIO_PIN_L_DIR 12
#define GPIO_PIN_R_DIR 13
#define GPIO_PIN_L_PWM 16
#define GPIO_PIN_R_PWM 17

bool motor_init()
{
    dev_gpio_set_mode(GPIO_PIN_EN, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_L_DIR, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_R_DIR, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_L_PWM, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_R_PWM, GPIO_FSEL_OUT);

    dev_gpio_set_pull(GPIO_PIN_EN, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_L_DIR, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_R_DIR, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_L_PWM, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_R_PWM, GPIO_PUD_DOWN);

    return true;
}

void motor_enable(bool enable)
{
    if (enable)
    {
        dev_gpio_set(1 << GPIO_PIN_EN);
    }
    else
    {
        dev_gpio_clear(1 << GPIO_PIN_EN);
    }
}

void motor_set_velocity(float vL, float vR)
{
    if (vR > 0)
    {
        dev_gpio_set(1 << GPIO_PIN_R_DIR);
        dev_gpio_clear(1 << GPIO_PIN_R_PWM);
    }
    else
    {
        dev_gpio_clear(1 << GPIO_PIN_R_DIR);
        dev_gpio_set(1 << GPIO_PIN_R_PWM);
    }
}