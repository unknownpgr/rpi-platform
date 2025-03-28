#pragma once

#include <stdint.h>

void sensor_init();
void sensor_read(uint16_t *sensor_data);
void sensor_read_one(uint16_t *sensor_data);
void sensor_calibrate();

void sensor_test_led();
void sensor_test_raw();
void sensor_test_calibration();