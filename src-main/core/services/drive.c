#include <services/drive.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <state.h>

#include <ports/motor.h>
#include <ports/log.h>
#include <ports/timer.h>

#include <services/encoder.h>
#include <services/sensor.h>
#include <services/vsense.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define NUM_SENSORS 16

typedef struct
{
    double kP;
    double kI;
    double kD;
    double target;
    double error_prev;
    double error_accum;
} pid_control_t;

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

    left = state->encoder_left;
    right = state->encoder_right;

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
                left = state->encoder_left;
                right = state->encoder_right;

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

#define WEIGHTED_SUM

#ifdef WEIGHTED_SUM
    double weighted_sum = 0;
    double weight_sum = 0;
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        double weight = sensor_values[i];
        if (ABS(sensor_positions[i] - prev_position) > 0.3)
        {
            weight = 0;
        }
        weighted_sum += weight * sensor_positions[i];
        weight_sum += weight;
    }

    if (weight_sum == 0)
    {
        return prev_position;
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
        likelihood += 4 * tmp * tmp;

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
}

#define STATE_NONE 0x00
#define STATE_ACCUM 0x01

#define MARK_NONE 0x00
#define MARK_RIGHT 0x01
#define MARK_LEFT 0x02
#define MARK_BOTH 0x03
#define MARK_CROSS 0x04

#define TRACK_STRAIGHT 0x00
#define TRACK_LEFT 0x01
#define TRACK_RIGHT 0x02

static double mark_threshold = 0.8;
static uint8_t mark_state = STATE_NONE;
static bool mark_is_left = false;
static bool mark_is_right = false;
static bool mark_accum[NUM_SENSORS] = {false};
static int both_count = 0;

static uint8_t mark_state_machine(double *sensor_values, double position)
{
    static double sensor_positions[NUM_SENSORS];

    // Initialize candidate positions and sensor positions
    static bool is_initialized = false;
    if (!is_initialized)
    {
        for (int i = 0; i < NUM_SENSORS; i++)
        {
            sensor_positions[i] = i * 2.0 / (NUM_SENSORS - 1) - 1.0;
        }
        is_initialized = true;
    }

    bool current_left = false;
    bool current_right = false;

    for (int i = 0; i < NUM_SENSORS; i++)
    {
        bool b = sensor_values[i] > mark_threshold;
        if (b)
        {
            mark_accum[i] = true;
            if (sensor_positions[i] < position - 0.25)
            {
                current_left = true;
                mark_is_left = true;
            }
            else if (sensor_positions[i] > position + 0.25)
            {
                current_right = true;
                mark_is_right = true;
            }
        }
    }

    switch (mark_state)
    {
    case STATE_NONE:
        if (current_left || current_right)
        {
            mark_state = STATE_ACCUM;
        }
        else
        {
            for (int i = 0; i < NUM_SENSORS; i++)
            {
                mark_accum[i] = false;
            }
            mark_is_left = false;
            mark_is_right = false;
        }
        break;
    case STATE_ACCUM:
        if (!current_left && !current_right)
        {
            mark_state = STATE_NONE;
            int count = 0;
            for (int i = 0; i < NUM_SENSORS; i++)
            {
                if (mark_accum[i])
                {
                    count++;
                }
            }
            if (count == NUM_SENSORS)
            {
                return MARK_CROSS;
            }
            if (mark_is_left && mark_is_right)
            {
                return MARK_BOTH;
            }
            else if (mark_is_left)
            {
                return MARK_LEFT;
            }
            else if (mark_is_right)
            {
                return MARK_RIGHT;
            }
        }
        break;
    }

    return MARK_NONE;
}

static double default_speed = 15;
static double default_curvature = 1.5;
static double acceleration = 20;

static double v_min = 3.7 * 8;
static double v_max = 4.2 * 8;

int32_t encoer_left_prev;
int32_t encoder_right_prev;
pid_control_t pid_left;
pid_control_t pid_right;
loop_t loop_motor;
loop_t loop_vsense;

void drive_setup()
{
    default_speed = 15;
    default_curvature = 1.5;
    acceleration = 20;

    pid_init(&pid_left);
    pid_init(&pid_right);

    pid_left.kP = 3.0;
    pid_right.kP = 3.0;

    pid_left.kI = 100.0;
    pid_right.kI = 100.0;

    // Initialize encoder values
    encoer_left_prev = state->encoder_left;
    encoder_right_prev = state->encoder_right;

    // Initialize state
    state->position = 0.0;
    state->speed = 0.0;
    state->track = TRACK_STRAIGHT;

    // Initialize mark state
    mark_state = STATE_NONE;
    both_count = 0;
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        mark_accum[i] = false;
    }

    // Initialize battery voltage
    double voltage = 0;
    for (int i = 0; i < 50; i++)
    {
        voltage = voltage * 0.9 + vsense_read() * 0.1;
        timer_sleep_ns(1000000); // 1m
    }
    state->battery_voltage = voltage;

    loop_init(&loop_motor, 1000000);    // 1ms
    loop_init(&loop_vsense, 100000000); // 100ms

    motor_set_velocity(0, 0);
    motor_enable(true);
    timer_sleep_ns(100000000); // 100ms
}

void drive_loop()
{
    uint32_t dt_ns;

    state->position = line_find_position(state->sensor_data, state->position);
    uint8_t mark = mark_state_machine(state->sensor_data, state->position);

    switch (mark)
    {
    case MARK_LEFT:
        print("MARK_LEFT");
        break;
    case MARK_RIGHT:
        print("MARK_RIGHT");
        break;
    case MARK_BOTH:
        print("MARK_BOTH");
        both_count++;

        if (both_count == 2)
        {
            default_speed = 0;
            acceleration = 40;
        }

        break;
    case MARK_CROSS:
        print("MARK_CROSS");
        break;
    }

    // VSense loop
    if (loop_update(&loop_vsense, &dt_ns))
    {
        // Update vsense value with IIR filter
        state->battery_voltage = state->battery_voltage * 0.99 + vsense_read() * 0.01;
    }

    // Motor control loop
    if (loop_update(&loop_motor, &dt_ns))
    {
        // Get dt (loop may not run at constant rate)
        double dt = dt_ns / 1e9;

        // Update PID targets based on position
        pid_left.target = -state->speed * (1.0 + state->position * default_curvature);
        pid_right.target = state->speed * (1.0 - state->position * default_curvature);

        // Update speed
        if (state->speed < default_speed)
        {
            state->speed += acceleration * dt;
            if (state->speed > default_speed)
            {
                state->speed = default_speed;
            }
        }
        else if (state->speed > default_speed)
        {
            state->speed -= acceleration * dt;
            if (state->speed < default_speed)
            {
                state->speed = default_speed;
            }
        }

        // Calculate speed
        double speed_left = (state->encoder_left - encoer_left_prev) / 4096.0 / dt;
        double speed_right = (state->encoder_right - encoder_right_prev) / 4096.0 / dt;

        // Update previous encoder value
        encoer_left_prev = state->encoder_left;
        encoder_right_prev = state->encoder_right;

        // Calculate PID
        double pid_left_output = pid_update(&pid_left, speed_left, dt);
        double pid_right_output = pid_update(&pid_right, speed_right, dt);

        // Update motor speed
        double motor_left_output = pid_left_output / state->battery_voltage;
        double motor_right_output = pid_right_output / state->battery_voltage;

        motor_set_velocity(motor_left_output, motor_right_output);
    }
}

void drive_teardown()
{
    motor_set_velocity(0, 0);
    motor_enable(false);
}

em_service_t service_drive = {
    .state_mask = EM_STATE_DRVIE,
    .setup = drive_setup,
    .loop = drive_loop,
    .teardown = drive_teardown,
};