#include <ports/motor.h>

#include <stdio.h>
#include <unistd.h>

#include <ports/dev.h>

#define GPIO_PIN_EN 18
#define GPIO_PIN_L_DIR 16
#define GPIO_PIN_R_DIR 17
#define GPIO_PIN_L_PWM 12 // PWM0_0
#define GPIO_PIN_R_PWM 13 // PWM0_1

#define MOTOR_PWM_RANGE 500
// PWM clock is 20MHz. Therefore the PWM frequency is 20MHz / 500 = 40kHz.
// Its way above the audible range, so it should be fine.

bool motor_init()
{
    dev_gpio_set_mode(GPIO_PIN_EN, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_L_DIR, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_R_DIR, GPIO_FSEL_OUT);
    dev_gpio_set_mode(GPIO_PIN_L_PWM, GPIO_FSEL_ALT0);
    dev_gpio_set_mode(GPIO_PIN_R_PWM, GPIO_FSEL_ALT0);

    dev_gpio_set_pull(GPIO_PIN_EN, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_L_DIR, GPIO_PUD_DOWN);
    dev_gpio_set_pull(GPIO_PIN_R_DIR, GPIO_PUD_DOWN);

    dev_pwm_set_range(0, MOTOR_PWM_RANGE);
    dev_pwm_set_range(1, MOTOR_PWM_RANGE);

    return true;
}

void motor_enable(bool enable)
{
    if (enable)
    {
        dev_gpio_set_pin(GPIO_PIN_EN);
    }
    else
    {
        dev_gpio_clear_pin(GPIO_PIN_EN);
    }
    dev_pwm_enable(0, enable);
    dev_pwm_enable(1, enable);
}

void motor_set_velocity(float vL, float vR)
{
    // Clip velocity
    if (vL > 0.95f)
    {
        vL = 0.95f;
    }
    else if (vL < -0.95f)
    {
        vL = -0.95f;
    }

    if (vR > 0.95f)
    {
        vR = 0.95f;
    }
    else if (vR < -0.95f)
    {
        vR = -0.95f;
    }

    if (vR > 0)
    {
        uint32_t data = (uint32_t)((1 - vR) * MOTOR_PWM_RANGE);
        dev_gpio_set_pin(GPIO_PIN_R_DIR);
        dev_pwm_set_data(1, data);
    }
    else
    {
        uint32_t data = (uint32_t)(vR * -MOTOR_PWM_RANGE);
        dev_gpio_clear_pin(GPIO_PIN_R_DIR);
        dev_pwm_set_data(1, data);
    }

    if (vL > 0)
    {
        uint32_t data = (uint32_t)((1 - vL) * MOTOR_PWM_RANGE);
        dev_gpio_set_pin(GPIO_PIN_L_DIR);
        dev_pwm_set_data(0, data);
    }
    else
    {
        uint32_t data = (uint32_t)(vL * -MOTOR_PWM_RANGE);
        dev_gpio_clear_pin(GPIO_PIN_L_DIR);
        dev_pwm_set_data(0, data);
    }
}