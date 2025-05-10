#include <algorithms/pid.h>

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