#define _GNU_SOURCE
#include <main.h>

#include <math.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <log.h>
#include <timer.h>
#include <motor.h>
#include <linalg.h>
#include <encoder.h>

#include <imu-service.h>
#include <music-service.h>
#include <vsense-service.h>
#include <sensor-service.h>
#include <keyboard-service.h>

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

void pin_thread_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

bool pseudo_random()
{
    // Implement pseudo-random binary sequence generator
    static uint32_t a = 0x7FFFFFFF; // Seed value

    // Implement PRBS-31 (x^31 + x^28 + 1)
    uint32_t b = ((a >> 30) ^ (a >> 27)) & 1;
    a = (a << 1) | b;
    a &= 0x7FFFFFFF; // Keep it 31 bits
    return b;
}

void tune_pid()
{
    print("Tuning PID...");

// System identification with ARX
#define N 10000
#define INPUT_MS 10

    // Collect data
    double u[N] = {0};
    uint32_t y[N] = {0};

    uint32_t _, right;
    uint32_t i = 0;
    double input = 0.0;
    double battery_voltage = 0.0;

    vsense_read();
    battery_voltage = vsense_read();

    encoder_get_counts(&_, &right);
    y[0] = right;

    loop_t loop;
    loop_init(&loop, 1000000); // 1ms
    uint32_t dt_ns;
    uint32_t j = 0;
    while (true)
    {
        if (loop_update(&loop, &dt_ns))
        {
            // Get y
            encoder_get_counts(&_, &right);
            y[i] = right;

            // Generate pseudo-random input
            if (j == INPUT_MS)
            {
                j = 0;
                // Generate pseudo-random input
                input = pseudo_random() ? 1.0 : -1.0;
                input *= 5;
                input /= battery_voltage; // Scale input by battery_voltage
            }
            else
            {
                j++;
            }

            motor_set_velocity(0, input);
            u[i] = input;
            i++;

            if (i >= N)
            {
                break;
            }

            // Read battery_voltage
            battery_voltage = vsense_read();
        }
    }
    motor_set_velocity(0, 0);

// Print data
#if 0
    printf("----------\n");
    for (uint32_t j = 0; j < N; j++)
    {
        printf("%f, %d\n", u[j], y[j]);
    }

    printf("----------\n");
#endif

#define ARX_ORDER 2
#define NUM_INPUTS 2

    // Convert encoder counts to speed
    double v[N] = {0};
    for (uint32_t j = 1; j < N; j++)
    {
        v[j] = (int32_t)(y[j] - y[j - 1]);
    }

    uint32_t n;
    if (INPUT_MS > ARX_ORDER)
    {
        n = INPUT_MS;
    }
    else
    {
        n = ARX_ORDER;
    }

    double *D = (double *)malloc(sizeof(double) * (N - n) * (ARX_ORDER + NUM_INPUTS));
    double *Y = (double *)malloc(sizeof(double) * (N - n));

    // Fill D and Y
    for (uint32_t j = 0; j < N - n; j++)
    {
        for (uint32_t k = 0; k < ARX_ORDER; k++)
        {
            D[j * (ARX_ORDER + NUM_INPUTS) + k] = v[j + k];
        }
        for (uint32_t k = 0; k < NUM_INPUTS; k++)
        {
            D[j * (ARX_ORDER + NUM_INPUTS) + ARX_ORDER + k] = u[j + k];
        }
        Y[j] = v[j + n];
    }

    // Solve the least squares problem
    double *x = (double *)malloc(sizeof(double) * (ARX_ORDER + NUM_INPUTS));

    pseudo_inverse(ARX_ORDER + NUM_INPUTS, N - n, D, Y, x);

    // Print results
    printf("ARX coefficients:\n");
    for (uint32_t j = 0; j < ARX_ORDER + NUM_INPUTS; j++)
    {
        printf("%f\n", x[j]);
    }

    // Free memory
    free(D);
    free(Y);
    free(x);
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

    clear();
    print("Ready to drive");
    print("Press any key to start");
    keyboard_wait_any_key();

    float default_speed = .5;
    float default_curvature = 3.5;

    pid_control_t pid_left, pid_right;
    pid_init(&pid_left);
    pid_init(&pid_right);

    pid_left.kP = 3.0;
    pid_right.kP = 3.0;

    pid_left.kI = 100.0;
    pid_right.kI = 100.0;

    int32_t left, right;
    int32_t left_prev, right_prev;

    // Initialize encoder values
    encoder_get_counts(&left, &right);
    left_prev = left;
    right_prev = right;

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
        // TODO: SPI is terribly slow. We need to optimize this.
        // About 10848 loops per second. (~11kHz)
        // Much lower then I expected. (~100kHz)

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