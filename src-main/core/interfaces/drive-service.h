#pragma once

#include <stdint.h>
#include <timer.h>

typedef struct
{
  double kP;
  double kI;
  double kD;
  double target;
  double error_prev;
  double error_accum;
} pid_control_t;

typedef struct
{
  pid_control_t pid_left;
  pid_control_t pid_right;
  int32_t left;
  int32_t right;
  int32_t left_prev;
  int32_t right_prev;
  loop_t loop_motor;
  loop_t loop_vsense;
  double position;
  double speed;
  double battery_voltage;
} drive_state_t;

void drive_init(drive_state_t *state);
void drive_loop();
