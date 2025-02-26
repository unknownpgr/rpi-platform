#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dev.h>
#include <motor.h>
#include <log.h>
#include <math.h>
#include <encoder.h>
#include <signal.h>

void signal_handler()
{
    // Disable motor
    motor_enable(false);

    // Disable PWM
    dev_pwm_disable(0, 0);
    dev_pwm_disable(0, 1);
    dev_pwm_disable(1, 0);
    dev_pwm_disable(1, 1);

    // Clear all GPIO
    dev_gpio_clear_mask(0xFFFFFFFFFFFFFFFF);

    // Exit program
    exit(0);
}

void set_signal_handler()
{
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGPIPE, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

void print_index(uint8_t index)
{
    index = index % 16;
    char buffer[17];
    for (uint8_t i = 0; i < 16; i++)
    {
        if (i == index)
            buffer[i] = 'X';
        else
            buffer[i] = '-';
    }
    buffer[16] = '\0';
    printf("Index: %02d %s\r", index, buffer);
    fflush(stdout);
}

int main()
{
    set_signal_handler();
    print("Program started");

    if (!dev_init())
    {
        print("Failed to initialize device");
        return 1;
    }
    print("Device initialized");

    if (!motor_init())
    {
        print("Failed to initialize motor");
        return 1;
    }
    print("Motor initialized");

    if (!encoder_init())
    {
        print("Failed to initialize encoder");
        return 1;
    }
    print("Encoder initialized");

    motor_enable(true);
    print("Motor enabled");

    float gain_p = 0.002;
    float gain_i = 0.00002;
    float gain_d = 0.01;

    int32_t div = 1024;

    float max_speed = 0.2;
    float error_accum = 0;
    float error_prev = 0;
    uint32_t target_position = 0;
    uint32_t current_position = 0;
    uint8_t index = 0;

    while (true)
    {
        encoder_update();
        encoder_get_counts(&current_position, &current_position);

        int32_t error = target_position - current_position;

        if (error > (div / 2))
        {
            target_position -= div;
            index--;
            print_index(index);
        }
        else if (error < -(div / 2))
        {
            target_position += div;
            index++;
            print_index(index);
        }

        error_accum += error;
        if (error_accum > 1000)
            error_accum = 1000;
        if (error_accum < -1000)
            error_accum = -1000;

        float error_diff = error - error_prev;
        error_prev = error;

        float speed = -(gain_p * error + gain_i * error_accum + gain_d * error_diff);
        if (speed > max_speed)
            speed = max_speed;
        if (speed < -max_speed)
            speed = -max_speed;
        motor_set_velocity(0, speed);
    }

    return 0;
}