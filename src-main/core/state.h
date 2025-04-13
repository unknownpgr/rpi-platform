#pragma once

#include <stdint.h>
#include <stdbool.h>

#define STATE_EXIT 0x00
#define STATE_IDLE 0x01
#define STATE_CALI_LOW 0x02
#define STATE_CALI_HIGH 0x03
#define STATE_DRIVE 0x04

#define TRACK_STRAIGHT 0x00
#define TRACK_LEFT 0x01
#define TRACK_RIGHT 0x02

#define NUM_SENSORS 16

typedef struct
{
  uint8_t state;

  // Sensor related
  uint16_t sensor_low[NUM_SENSORS];
  uint16_t sensor_high[NUM_SENSORS];
  uint16_t sensor_raw[NUM_SENSORS];
  double sensor_data[NUM_SENSORS];

  // Drive related
  double position;
  double speed;
  double battery_voltage;
  uint8_t track;
  int encoder_left;
  int encoder_right;
} state_t;

extern state_t *state;