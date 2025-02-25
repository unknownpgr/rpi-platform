#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_PIN_EN 18
#define GPIO_PIN_L_DIR 12
#define GPIO_PIN_R_DIR 13
#define GPIO_PIN_L_PWM 16
#define GPIO_PIN_R_PWM 17

#define PWM_FREQUENCY 1000

typedef struct gpiod_chip *gpio_chip_t;
typedef struct gpiod_line *gpio_line_t;

gpio_chip_t chip;
gpio_line_t pin_en;
gpio_line_t pin_l_dir;
gpio_line_t pin_l_pwm;
gpio_line_t pin_r_dir;
gpio_line_t pin_r_pwm;

bool motor_init()
{
    // Get a GPIO chip
    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip)
    {
        perror("Failed to open GPIO chip");
        return false;
    }

    // Get GPIO lines
    pin_l_dir = gpiod_chip_get_line(chip, GPIO_PIN_L_DIR);
    pin_l_pwm = gpiod_chip_get_line(chip, GPIO_PIN_L_PWM);
    pin_r_dir = gpiod_chip_get_line(chip, GPIO_PIN_R_DIR);
    pin_r_pwm = gpiod_chip_get_line(chip, GPIO_PIN_R_PWM);
    pin_en = gpiod_chip_get_line(chip, GPIO_PIN_EN);
    if (!pin_l_dir || !pin_l_pwm || !pin_r_dir || !pin_r_pwm || !pin_en)
    {
        perror("Failed to get GPIO line");
        gpiod_chip_close(chip);
        return false;
    }

    // Set GPIO lines as output
    if (gpiod_line_request_output(pin_l_dir, "l_dir", 0) < 0 ||
        gpiod_line_request_output(pin_l_pwm, "l_pwm", 0) < 0 ||
        gpiod_line_request_output(pin_r_dir, "r_dir", 0) < 0 ||
        gpiod_line_request_output(pin_r_pwm, "r_pwm", 0) < 0 ||
        gpiod_line_request_output(pin_en, "en", 0) < 0)
    {
        perror("Failed to set GPIO as output");
        gpiod_chip_close(chip);
        return false;
    }

    return true;
}

void motor_enable(bool enable)
{
    gpiod_line_set_value(pin_en, enable);
}

void motor_set_velocity(float vL, float vR)
{
    // Set motor direction
    gpiod_line_set_value(pin_l_dir, vL >= 0);
    gpiod_line_set_value(pin_r_dir, vR >= 0);

    // Set motor speed
}