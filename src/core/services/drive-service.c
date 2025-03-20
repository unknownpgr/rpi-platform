#include <drive-service.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <encoder.h>
#include <motor.h>
#include <timer.h>
#include <log.h>
#include <keyboard-service.h>

typedef struct
{
    uint32_t interval_ns;
    uint32_t last_time_ns;
} interval_t;

static interval_t create_interval(uint32_t interval_ns)
{
    interval_t interval = {
        .interval_ns = interval_ns,
        .last_time_ns = timer_get_ns(),
    };
    return interval;
}

#define ON(interval, code)                                                          \
    if (DIFF(timer_get_ns(), interval.last_time_ns) > interval.interval_ns)         \
    {                                                                               \
        interval.last_time_ns = ACCUM(interval.last_time_ns, interval.interval_ns); \
        {code};                                                                     \
    }

static double clip(double value, double abs_max)
{
    if (value > abs_max)
        return abs_max;
    if (value < -abs_max)
        return -abs_max;
    return value;
}

void drive_test()
{
    motor_enable(true);

    const uint32_t ns_1ms = 1000000; // 1ms
    interval_t interval_motor = create_interval(ns_1ms);

    const uint32_t ns_100ms = 50000000; // 100ms
    interval_t interval_keyboard = create_interval(ns_100ms);

    uint32_t last_key_pressed = 0;

    uint32_t position_l;
    uint32_t position_r;
    uint32_t position_l_prev = 0;
    uint32_t position_r_prev = 0;
    encoder_get_counts(&position_l_prev, &position_r_prev);

    float vL = 0;
    float vR = 0;

    while (true)
    {
        encoder_update();

        ON(interval_keyboard, {
            char key = keyboard_get_key();

            if (DIFF(timer_get_ns(), last_key_pressed) > 550000000)
            {
                vL = 0;
                vR = 0;
            }

            switch (key)
            {
            case KEY_W:
                vL = 0.5;
                vR = 0.5;
                last_key_pressed = timer_get_ns();
                break;
            case KEY_S:
                vL = -0.5;
                vR = -0.5;
                last_key_pressed = timer_get_ns();
                break;
            case KEY_A:
                vL = -0.5;
                vR = 0.5;
                last_key_pressed = timer_get_ns();
                break;
            case KEY_D:
                vL = 0.5;
                vR = -0.5;
                last_key_pressed = timer_get_ns();
                break;
            case KEY_SPACE:
                vL = 0;
                vR = 0;
                last_key_pressed = timer_get_ns();
                break;
            default:
                break;
            }
        })

        encoder_update();

        ON(interval_motor, {
            // Get current encoder position
            encoder_get_counts(&position_l, &position_r);

            // Get difference in encoder position
            int32_t diff_l = position_l - position_l_prev;
            int32_t diff_r = position_r - position_r_prev;

            // Update previous encoder position
            position_l_prev = position_l;
            position_r_prev = position_r;

            // Calculate motor output
            float output_l = (vL - diff_l / 490.6f);
            float output_r = (vR - diff_r / 490.6f);

            // Clip motor output
            output_l = clip(output_l, 0.5);
            output_r = clip(output_r, 0.5);

            // Set motor velocity
            motor_set_velocity(-output_l, output_r);
        })
    }

    return;
}