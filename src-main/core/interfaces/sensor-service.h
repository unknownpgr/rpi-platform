#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NUM_SENSORS 16

#define STATE_READING 0
#define STATE_CALI_LOW 1
#define STATE_CALI_HIGH 2

typedef struct
{
  uint16_t sensor_low[NUM_SENSORS];
  uint16_t sensor_high[NUM_SENSORS];
} calibration_t;

typedef struct
{
  uint8_t state;
  bool is_calibrated;
  uint16_t raw_data[NUM_SENSORS];
  double sensor_data[NUM_SENSORS];
  calibration_t calibration;
  uint8_t sensor_index;
} sensor_state_t;

extern sensor_state_t *sensor_state;

void sensor_init(sensor_state_t *state);
void sensor_load_calibration();
void sensor_save_calibration();
void sensor_loop();

void sensor_test_led();
void sensor_test_raw();
void sensor_test_calibration();