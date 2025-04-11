#pragma once

#include <stdint.h>
#include <stdbool.h>
void sensor_init();
void sensor_load_calibration();
bool sensor_is_calibrated();

void sensor_read_one(double *sensor_data);
void sensor_calibrate();

void sensor_test_led();
void sensor_test_raw();
void sensor_test_calibration();