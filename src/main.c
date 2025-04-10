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

bool tune_pseudo_random_bit_sequence()
{
    // Implement pseudo-random binary sequence generator
    static uint32_t a = 0x7FFFFFFF; // Seed value

    // Implement PRBS-31 (x^31 + x^28 + 1)
    uint32_t b = ((a >> 30) ^ (a >> 27)) & 1;
    a = (a << 1) | b;
    a &= 0x7FFFFFFF; // Keep it 31 bits
    return b;
}

void tune_collect_motor_response(double *input, double *output, uint32_t n, uint32_t dt_ns, bool is_left)
{
    double max_input = 5.0;

    motor_enable(true);
    motor_set_velocity(0, 0);

    loop_t loop;
    loop_init(&loop, dt_ns);

    uint32_t previous_position = 0;
    uint32_t left, right;

    encoder_get_counts(&left, &right);
    if (is_left)
    {
        previous_position = left;
    }
    else
    {
        previous_position = right;
    }

    double battery_voltage = 0.0;
    vsense_read();
    battery_voltage = vsense_read();
    print("Battery Voltage: %2.2f", battery_voltage);

    uint32_t i = 0;
    uint32_t _;

    double input_value = 0.0;

    while (true)
    {
        if (loop_update(&loop, &_))
        {
            if (i % 10 == 0)
            {
                input_value = tune_pseudo_random_bit_sequence() ? max_input : -max_input;
                input_value /= battery_voltage;
            }

            // Output
            {
                encoder_get_counts(&left, &right);
                if (is_left)
                {
                    output[i] = (double)(int32_t)(left - previous_position);
                    previous_position = left;
                }
                else
                {
                    output[i] = (double)(int32_t)(right - previous_position);
                    previous_position = right;
                }
            }

            // Input
            {
                if (is_left)
                {
                    motor_set_velocity(input_value, 0);
                }
                else
                {
                    motor_set_velocity(0, input_value);
                }
                input[i] = input_value;
            }

            i++;
            if (i >= n)
            {
                break;
            }

            // Read battery_voltage
            battery_voltage = vsense_read() * 0.1 + battery_voltage * 0.9;
        }
    }

    // Stop motor
    motor_set_velocity(0, 0);
    motor_enable(false);
}

void tune_collect()
{
    // System identification with ARX model
    print("Tuning PID...");

#define N 10000
#define DT_NS 1000000 // 1ms
#define ARX_ORDER 2

    double u[N] = {0};
    double v[N] = {0};

    // Collect motor response of left motor
    tune_collect_motor_response(u, v, N, DT_NS, true);

    // Print results for left motor
    printf("----------\n");
    fflush(stdout);
    for (uint32_t i = 0; i < N; i++)
    {
        printf("%f, %f\n", u[i], v[i]);
        fflush(stdout);
    }

    printf("----------\n");
    fflush(stdout);

    // Collect motor response of right motor
    tune_collect_motor_response(u, v, N, DT_NS, true);

    // Print results for right motor
    fflush(stdout);
    for (uint32_t i = 0; i < N; i++)
    {
        printf("%f, %f\n", u[i], v[i]);
        fflush(stdout);
    }
    printf("----------\n");
    fflush(stdout);
}

void pin_thread_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

double line_mu(double distance)
{
#define ABS(x) ((x) < 0 ? -(x) : (x))

    double mu = 1 - 3 * ABS(distance);
    if (mu < 0)
    {
        mu = 0;
    }
    return mu;

#undef ABS
}

double line_find_position(double *sensor_values, double prev_position)
{
#define CANDIDATES 1000
#define NUM_SENSORS 16

    static double candidate_positions[CANDIDATES];
    static double sensor_positions[NUM_SENSORS];

    // Initialize candidate positions and sensor positions
    {
        static bool is_initialized = false;
        if (!is_initialized)
        {
            for (int i = 0; i < CANDIDATES; i++)
            {
                candidate_positions[i] = i * 2.0 / CANDIDATES - 1.0;
            }
            for (int i = 0; i < NUM_SENSORS; i++)
            {
                sensor_positions[i] = i * 2.0 / NUM_SENSORS - 1.0;
            }
            is_initialized = true;
        }
    }

    double optimal_likelihood = 999999999;
    double optimal_position = 0;

    for (int i = 0; i < CANDIDATES; i++)
    {
        double candidate_position = candidate_positions[i];
        double likelihood = 0;

        // Set the prior
        likelihood = -line_mu(candidate_position - prev_position);

        // Add evidence
        for (int j = 0; j < NUM_SENSORS; j++)
        {
            double tmp = sensor_values[j] - line_mu(candidate_position - sensor_positions[j]);
            likelihood += tmp * tmp;
        }

        // Check if this is the best position
        if (likelihood < optimal_likelihood)
        {
            optimal_likelihood = likelihood;
            optimal_position = candidate_position;
        }
    }

    return optimal_position;

#undef CANDIDATES
#undef NUM_SENSORS
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

    tune_collect();
    while (1)
        ;

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