#include <knob-service.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <encoder.h>
#include <motor.h>

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

void knob_test()
{
    float gain_p = 0.002;
    float gain_i = 0.00002;
    float gain_d = 0.0001;

    int32_t div = 1024;

    float max_speed = 0.15;
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
        if (error_accum > 5000)
            error_accum = 5000;
        if (error_accum < -5000)
            error_accum = -5000;

        float error_diff = error - error_prev;
        error_prev = error;

        float speed = -(gain_p * error + gain_i * error_accum + gain_d * error_diff);
        if (speed > max_speed)
            speed = max_speed;
        if (speed < -max_speed)
            speed = -max_speed;
        motor_set_velocity(0, speed);
    }
}