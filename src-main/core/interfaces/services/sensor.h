#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <state.h>

void sensor_init();
void sensor_load_calibration();
void sensor_save_calibration();
void sensor_loop();

void sensor_test_led();
void sensor_test_raw();
void sensor_test_calibration();