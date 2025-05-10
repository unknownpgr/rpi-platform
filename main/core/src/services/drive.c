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

#include <algorithms/pid.h>
#include <algorithms/mark.h>

#define TRACK_STRAIGHT 0x00
#define TRACK_LEFT 0x01
#define TRACK_RIGHT 0x02

static double default_speed = 15;
static double default_curvature = 1.5;
static double acceleration = 20;

int32_t encoer_left_prev;
int32_t encoder_right_prev;
pid_control_t pid_left;
pid_control_t pid_right;
loop_t loop_motor;
mark_t mark;
int both_count = 0;

void drive_setup()
{
    default_speed = 15;
    default_curvature = 1.5;
    acceleration = 20;
    both_count = 0;

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

    mark_init(&mark);
    loop_init(&loop_motor, 1000000); // 1ms

    motor_set_velocity(0, 0);
    motor_enable(true);
}

void drive_loop()
{
    uint32_t dt_ns;
    uint8_t current_mark = mark_state_machine(&mark, state->sensor_data, state->position);

    switch (current_mark)
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

    // Motor control loop
    if (!loop_update(&loop_motor, &dt_ns))
        return;

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

void drive_teardown()
{
    motor_set_velocity(0, 0);
    motor_enable(false);
}

em_service_t service_drive = {
    .state_mask = EM_STATE_DRIVE,
    .setup = drive_setup,
    .loop = drive_loop,
    .teardown = drive_teardown,
};