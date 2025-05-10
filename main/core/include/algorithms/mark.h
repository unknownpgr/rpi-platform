#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NUM_SENSORS 16

#define MARK_NONE 0x00
#define MARK_RIGHT 0x01
#define MARK_LEFT 0x02
#define MARK_BOTH 0x03
#define MARK_CROSS 0x04

typedef struct
{
  uint8_t state;
  bool is_left;
  bool is_right;
  bool accum[NUM_SENSORS];
} mark_t;

void mark_init(mark_t *mark);
uint8_t mark_state_machine(mark_t *mark, double *sensor_data, double position);
