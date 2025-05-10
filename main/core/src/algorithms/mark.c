#include <algorithms/mark.h>

#define STATE_NONE 0x00
#define STATE_ACCUM 0x01

static double mark_threshold = 0.8;

static double sensor_positions[NUM_SENSORS];

void mark_init(mark_t *mark)
{
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    sensor_positions[i] = i * 2.0 / (NUM_SENSORS - 1) - 1.0; // -1.0 ~ 1.0
  }

  mark->state = STATE_NONE;
  mark->is_left = false;
  mark->is_right = false;
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    mark->accum[i] = false;
  }
}

uint8_t mark_state_machine(mark_t *mark, double *sensor_data, double position)
{
  bool current_left = false;
  bool current_right = false;

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    bool b = sensor_data[i] > mark_threshold;
    if (b)
    {
      mark->accum[i] = true;
      if (sensor_positions[i] < position - 0.25)
      {
        current_left = true;
        mark->is_left = true;
      }
      else if (sensor_positions[i] > position + 0.25)
      {
        current_right = true;
        mark->is_right = true;
      }
    }
  }

  switch (mark->state)
  {
  case STATE_NONE:
    if (current_left || current_right)
    {
      mark->state = STATE_ACCUM;
    }
    else
    {
      for (int i = 0; i < NUM_SENSORS; i++)
      {
        mark->accum[i] = false;
      }
      mark->is_left = false;
      mark->is_right = false;
    }
    break;
  case STATE_ACCUM:
    if (!current_left && !current_right)
    {
      mark->state = STATE_NONE;
      int count = 0;
      for (int i = 0; i < NUM_SENSORS; i++)
      {
        if (mark->accum[i])
        {
          count++;
        }
      }
      if (count == NUM_SENSORS)
      {
        return MARK_CROSS;
      }
      if (mark->is_left && mark->is_right)
      {
        return MARK_BOTH;
      }
      else if (mark->is_left)
      {
        return MARK_LEFT;
      }
      else if (mark->is_right)
      {
        return MARK_RIGHT;
      }
    }
    break;
  }

  return MARK_NONE;
}
