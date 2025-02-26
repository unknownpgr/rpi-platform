#include <stdio.h>
#include <unistd.h>
#include <dev.h>
#include <motor.h>
#include <log.h>
#include <math.h>
#include <encoder.h>

int main()
{
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