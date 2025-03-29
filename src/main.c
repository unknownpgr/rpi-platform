#define _GNU_SOURCE
#include <main.h>

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

#include <log.h>
#include <timer.h>
#include <motor.h>
#include <encoder.h>

#include <imu-service.h>
#include <vsense-service.h>
#include <sensor-service.h>

typedef struct
{
    double kP;
    double kI;
    double kD;
    double target;
    double error_prev;
    double error_accum;
} pid_control_t;

void pid_init(pid_control_t *pid)
{
    pid->kP = 0.0;
    pid->kI = 0.0;
    pid->kD = 0.0;
    pid->target = 0.0;
    pid->error_prev = 0.0;
    pid->error_accum = 0.0;
}

double pid_update(pid_control_t *pid, double current, double dt)
{
    double error = pid->target - current;
    pid->error_accum += error * dt;
    double derivative = (error - pid->error_prev) / dt;
    pid->error_prev = error;
    return pid->kP * error + pid->kI * pid->error_accum + pid->kD * derivative;
}

typedef struct
{
    uint32_t interval_ns;
    uint32_t last_time_ns;
} loop_t;

void loop_init(loop_t *loop, uint32_t interval_ns)
{
    loop->interval_ns = interval_ns;
    loop->last_time_ns = timer_get_ns();
}

bool loop_update(loop_t *loop, uint32_t *dt_ns)
{
    uint32_t current_time = timer_get_ns();
    *dt_ns = DIFF(current_time, loop->last_time_ns);
    if (*dt_ns >= loop->interval_ns)
    {
        loop->last_time_ns = current_time;
        return true;
    }
    return false;
}

void pin_thread_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

void thread_timer(void *_)
{
    print("Timer thread started");
    while (true)
    {
        usleep(10000); // Sleep for 10ms
        timer_update();
    }
}

void thread_encoder(void *_)
{
    print("Encoder thread started");
    pin_thread_to_cpu(2);

    loop_t loop_encoder;
    uint32_t dt_ns;
    loop_init(&loop_encoder, 1000); // 1us
    while (true)
    {
        if (loop_update(&loop_encoder, &dt_ns))
        {
            encoder_update();
        }
    }
}

void thread_drive(void *_)
{
    print("Drive thread started");
    pin_thread_to_cpu(3);

    motor_enable(true);
    timer_sleep_ns(100000000); // 100ms

    sensor_calibrate();
    print("Calibration done");

    float default_speed = 2.5;
    float default_curvature = 1.5;

    pid_control_t pid_left, pid_right;
    pid_init(&pid_left);
    pid_init(&pid_right);

    pid_left.kP = 3.0;
    pid_right.kP = 3.0;

    pid_left.kI = 100.0;
    pid_right.kI = 100.0;

    int32_t left, right;
    int32_t left_prev, right_prev;

    loop_t loop_motor;
    loop_t loop_vsense;

    loop_init(&loop_motor, 1000000);    // 1ms
    loop_init(&loop_vsense, 100000000); // 100ms

    vsense_read(); // Select channel
    double vsense = vsense_read();
    double v_min = 3.7 * 8;
    double v_max = 4.2 * 8;
    print("Battery Voltage: %2.2f, %2.2f%%", vsense, (vsense - v_min) / (v_max - v_min) * 100);

    double sensor_values[16] = {0};
    double sensor_positions[16] = {-1.5, -1.3, -1.1, -0.9, -0.7, -0.5, -0.3, -0.1, 0.1, 0.3, 0.5, 0.7, 0.9, 1.1, 1.3, 1.5};
    double position = 0;

    uint32_t dt_ns;
    while (true)
    {
        // Read sensor values
        sensor_read_one(sensor_values);

        double weight_sum = 0;
        double weighted_value_sum = 0;
        for (int i = 0; i < 16; i++)
        {
            weight_sum += sensor_values[i];
            weighted_value_sum += sensor_values[i] * sensor_positions[i];
        }
        if (weight_sum > 0)
        {
            position = weighted_value_sum / weight_sum;
        }

        // VSense loop
        if (loop_update(&loop_vsense, &dt_ns))
        {
            // Update vsense value with IIR filter
            vsense = vsense * 0.5 + vsense_read() * 0.5;
        }

        // Motor control loop
        if (loop_update(&loop_motor, &dt_ns))
        {
            // Get dt (loop may not run at constant rate)
            double dt = dt_ns / 1e9;

            // Update PID targets based on position
            pid_left.target = -default_speed * (1.0 + position * default_curvature);
            pid_right.target = default_speed * (1.0 - position * default_curvature);

            // Calculate speed
            encoder_get_counts(&left, &right);
            double speed_left = (left - left_prev) / 4096.0 / dt;
            double speed_right = (right - right_prev) / 4096.0 / dt;

            // Update previous encoder value
            left_prev = left;
            right_prev = right;

            // Calculate PID
            double pid_left_output = pid_update(&pid_left, speed_left, dt);
            double pid_right_output = pid_update(&pid_right, speed_right, dt);

            // Update motor speed
            motor_set_velocity(pid_left_output / vsense, pid_right_output / vsense);
        }
    }
}

int application_start()
{
    print("Program started");

    // Initialize peripherals
    imu_init();
    motor_init();
    vsense_init();
    encoder_init();
    sensor_init();
    timer_init();
    print("Peripherals initialized");

    int ret;

    // Start timer thread
    pthread_t timer_thread;
    ret = pthread_create(&timer_thread, NULL, (void *)thread_timer, NULL);
    if (ret != 0)
    {
        print("Error creating timer thread: %d", ret);
        return -1;
    }

    // Start encoder thread
    pthread_t encoder_thread;
    ret = pthread_create(&encoder_thread, NULL, (void *)thread_encoder, NULL);
    if (ret != 0)
    {
        print("Error creating encoder thread: %d", ret);
        return -1;
    }

    // Start drive thread
    pthread_t drive_thread;
    ret = pthread_create(&drive_thread, NULL, (void *)thread_drive, NULL);
    if (ret != 0)
    {
        print("Error creating drive thread: %d", ret);
        return -1;
    }

    // Join threads
    pthread_join(encoder_thread, NULL);
    pthread_join(drive_thread, NULL);
    print("All threads joined");

    return 0;
}