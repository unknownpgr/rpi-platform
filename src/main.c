#include <main.h>

#include <stdbool.h>
#include <stdint.h>

#include <log.h>
#include <timer.h>
#include <motor.h>

#include <imu-service.h>

uint32_t dt_ns()
{
    static bool is_initialized = false;
    static uint32_t last_time = 0;

    if (is_initialized)
    {
        uint32_t current_time = timer_get_ns();
        uint32_t delta_time = DIFF(current_time, last_time);
        last_time = current_time;
        return delta_time;
    }
    else
    {
        last_time = timer_get_ns();
        is_initialized = true;
        return 0;
    }
}

int application_start()
{
    print("Program started");
    imu_init();
    motor_init();
    motor_enable(true);

    const double Kp = 2.0;
    const double Ki = 15.0;
    const double Kd = 0.02;

    imu_data_t data;
    double angle = 0;
    double integral = 0;

    while (true)
    {
        imu_read(&data);
        double dt = dt_ns() / 1e9;

        double angularVelocity = data.gz * 0.01;
        angle += angularVelocity * dt;
        integral += angle * dt;

        double output = Kp * angle + Ki * integral + Kd * angularVelocity;
        motor_set_velocity(output, output);
        timer_sleep_ns(1000000); // 1ms
    }

    return 0;
}