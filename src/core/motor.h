#pragma once

#include <stdbool.h>

bool motor_init();
void motor_enable(bool enable);
void motor_set_velocity(float vL, float vR);