#include <drive-service.h>

#include <stdio.h>
#include <motor.h>
#include <encoder.h>
#include <log.h>

#include <sensor-service.h>
#include <vsense-service.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

static void pid_init(pid_control_t *pid)
{
    pid->kP = 0.0;
    pid->kI = 0.0;
    pid->kD = 0.0;
    pid->target = 0.0;
    pid->error_prev = 0.0;
    pid->error_accum = 0.0;
}

static double pid_update(pid_control_t *pid, double current, double dt)
{
    double error = pid->target - current;
    pid->error_accum += error * dt;
    double derivative = (error - pid->error_prev) / dt;
    pid->error_prev = error;
    return pid->kP * error + pid->kI * pid->error_accum + pid->kD * derivative;
}

static bool tune_pseudo_random_bit_sequence()
{
    // Implement pseudo-random binary sequence generator
    static uint32_t a = 0x7FFFFFFF; // Seed value

    // Implement PRBS-31 (x^31 + x^28 + 1)
    uint32_t b = ((a >> 30) ^ (a >> 27)) & 1;
    a = (a << 1) | b;
    a &= 0x7FFFFFFF; // Keep it 31 bits
    return b;
}

static void tune_collect_motor_response(double *input, double *output, uint32_t n, uint32_t dt_ns, bool is_left)
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

static void tune_collect()
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
    tune_collect_motor_response(u, v, N, DT_NS, false);

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

static double line_mu(double distance)
{
    double mu = 1 - 3 * ABS(distance);
    if (mu < 0)
    {
        mu = 0;
    }
    return mu;
}

static double line_find_position(double *sensor_values, double prev_position)
{
#define NUM_CANDIDATES 1000
#define NUM_SENSORS 16

    static double candidate_positions[NUM_CANDIDATES];
    static double sensor_positions[NUM_SENSORS];

    // Initialize candidate positions and sensor positions
    static bool is_initialized = false;
    if (!is_initialized)
    {
        for (int i = 0; i < NUM_CANDIDATES; i++)
        {
            candidate_positions[i] = i * 2.0 / (NUM_CANDIDATES - 1) - 1.0;
        }
        for (int i = 0; i < NUM_SENSORS; i++)
        {
            sensor_positions[i] = i * 2.0 / (NUM_SENSORS - 1) - 1.0;
        }
        is_initialized = true;
    }

#define WEIGHTED_SUM_NO

#ifdef WEIGHTED_SUM
    double weighted_sum = 0;
    double weight_sum = 0;
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        double weight = sensor_values[i];
        weighted_sum += weight * sensor_positions[i];
        weight_sum += weight;
    }
    double expected_position = weighted_sum / weight_sum;
    return expected_position;
#else
    double optimal_likelihood = 999999999;
    double optimal_position = 0;

    for (int i = 0; i < NUM_CANDIDATES; i++)
    {
        double candidate_position = candidate_positions[i];
        double likelihood = 0;

        // Set the prior
        double tmp = candidate_position - prev_position;
        likelihood += ABS(tmp) * 4;

        // Add evidence
        for (int j = 0; j < NUM_SENSORS; j++)
        {
            double tmp = sensor_values[j] - line_mu(candidate_position - sensor_positions[j]);
            // tmp는 예측값과 실제 값의 차이다. 즉, 그 절댓값이 클수록 예측값과 실제 값이 다르다는 의미다.
            // 따라서 이 값을 최소화하는 곳을 찾아야 한다.
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
#endif
#undef CANDIDATES
#undef NUM_SENSORS
}

const float default_speed = 5;
const float default_curvature = 3.5;

const double v_min = 3.7 * 8;
const double v_max = 4.2 * 8;

static drive_state_t *drive_state;

void drive_init(drive_state_t *state)
{
    drive_state = state;

    pid_init(&drive_state->pid_left);
    pid_init(&drive_state->pid_right);

    drive_state->pid_left.kP = 3.0;
    drive_state->pid_right.kP = 3.0;

    drive_state->pid_left.kI = 100.0;
    drive_state->pid_right.kI = 100.0;

    // Initialize encoder values
    encoder_get_counts(&drive_state->left, &drive_state->right);
    drive_state->left_prev = drive_state->left;
    drive_state->right_prev = drive_state->right;
    drive_state->position = 0.0;

    // Initialize battery voltage
    double voltage = 0;
    for (int i = 0; i < 50; i++)
    {
        voltage = voltage * 0.9 + vsense_read() * 0.1;
        timer_sleep_ns(1000000); // 1m
    }
    drive_state->battery_voltage = voltage;

    loop_init(&drive_state->loop_motor, 1000000);    // 1ms
    loop_init(&drive_state->loop_vsense, 100000000); // 100ms

    motor_enable(true);
    timer_sleep_ns(100000000); // 100ms
}

int counter = 0;

void drive_loop()
{
    uint32_t dt_ns;

    drive_state->position = line_find_position(sensor_state->sensor_data, drive_state->position);

    // VSense loop
    if (loop_update(&drive_state->loop_vsense, &dt_ns))
    {
        // Update vsense value with IIR filter
        drive_state->battery_voltage = drive_state->battery_voltage * 0.5 + vsense_read() * 0.5;
    }

    // Motor control loop
    if (loop_update(&drive_state->loop_motor, &dt_ns))
    {
        // Get dt (loop may not run at constant rate)
        double dt = dt_ns / 1e9;

        // Update PID targets based on position
        drive_state->pid_left.target = -default_speed * (1.0 + drive_state->position * default_curvature);
        drive_state->pid_right.target = default_speed * (1.0 - drive_state->position * default_curvature);

        // Calculate speed
        encoder_get_counts(&drive_state->left, &drive_state->right);
        double speed_left = (drive_state->left - drive_state->left_prev) / 4096.0 / dt;
        double speed_right = (drive_state->right - drive_state->right_prev) / 4096.0 / dt;

        // Update previous encoder value
        drive_state->left_prev = drive_state->left;
        drive_state->right_prev = drive_state->right;

        // Calculate PID
        double pid_left_output = pid_update(&drive_state->pid_left, speed_left, dt);
        double pid_right_output = pid_update(&drive_state->pid_right, speed_right, dt);

        // Update motor speed
        double motor_left_output = pid_left_output / drive_state->battery_voltage;
        double motor_right_output = pid_right_output / drive_state->battery_voltage;

        motor_set_velocity(motor_left_output, motor_right_output);
    }
}