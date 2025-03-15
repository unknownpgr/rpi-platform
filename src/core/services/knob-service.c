#include <knob-service.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <encoder.h>
#include <motor.h>
#include <timer.h>
#include <log.h>

typedef struct
{
    uint32_t interval_ns;
    uint32_t last_time_ns;
} interval_t;

interval_t create_interval(uint32_t interval_ns)
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

double clip(double value, double abs_max)
{
    if (value > abs_max)
        return abs_max;
    if (value < -abs_max)
        return -abs_max;
    return value;
}

void knob_test()
{
    {
        motor_enable(true);

        const uint32_t dt_ns = 1000000; // 1ms
        interval_t interval = create_interval(dt_ns);
        uint32_t target_position = 0;

        while (true)
        {
            encoder_update();
            ON(interval, {
                uint32_t position;
                encoder_get_counts(&position, &position);

                int32_t diff = target_position - position;

                if (diff > 500)
                {
                    target_position -= 1000;
                }
                else if (diff < -500)
                {
                    target_position += 1000;
                }

                double output = diff * 0.05;

                output = clip(output, 0.2);
                motor_set_velocity(0, output);
            })
        }

        return;
    }

    //================================================================================================

    // Constants
    const uint32_t dt_ns = 100000;

    const double kP = 0.05;
    const double kI = 0.7;
    const double kD = 0.001;

    // Time measurement
    uint32_t last_time = timer_get_ns();

    uint32_t target_position = 0;
    uint32_t current_position;
    double error_accumulated = 0;
    double error_previous = 0;

    motor_enable(true);
    interval_t interval = create_interval(dt_ns);
    while (true)
    {
        ON(interval, {
            // Get time difference
            uint32_t current_time = timer_get_ns();
            double dt_s = DIFF(current_time, last_time) / 1e9;
            last_time = current_time;

            // Get current position
            encoder_get_counts(&current_position, &current_position);

            // Calculate error
            int32_t error = target_position - current_position;        // P
            error_accumulated += error * dt_s;                         // I
            error_accumulated = clip(error_accumulated, 1);            // Anti-windup
            double error_derivative = (error - error_previous) / dt_s; // D
            error_previous = error;

            // Calculate output
            double output = -(kP * error + kI * error_accumulated + kD * error_derivative);
            output = clip(output, 0.5);

            // Apply output
            motor_set_velocity(0, output);
        })

        encoder_update();
    }
}