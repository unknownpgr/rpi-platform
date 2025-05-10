#pragma once

typedef struct
{
  double kP;
  double kI;
  double kD;
  double target;
  double error_prev;
  double error_accum;
} pid_control_t;

void pid_init(pid_control_t *pid);
double pid_update(pid_control_t *pid, double current, double dt);
