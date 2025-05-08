static bool tune_pseudo_random_bit_sequence()
{
  // Implement pseudo-random binary sequence generator
  static uint32_t a = 0x7FFFFFFF; // Seed value

  // Implement PRBS-31 (x^31 + x^28 + 1)
  uint32_t b = ((a >> 30) ^ (a >> 27)) & 1;
  a = (a << 1) | b;
  a &= 0x7FFFFFFF; // Keep it 31 bits
  return b;
}

static void tune_collect_motor_response(double *input, double *output, uint32_t n, uint32_t dt_ns, bool is_left)
{
  double max_input = 5.0;

  motor_enable(true);
  motor_set_velocity(0, 0);

  loop_t loop;
  loop_init(&loop, dt_ns);

  uint32_t previous_position = 0;
  uint32_t left, right;

  left = state->encoder_left;
  right = state->encoder_right;

  if (is_left)
  {
    previous_position = left;
  }
  else
  {
    previous_position = right;
  }

  uint32_t i = 0;
  uint32_t _;

  double input_value = 0.0;

  while (true)
  {
    if (loop_update(&loop, &_))
    {
      if (i % 10 == 0)
      {
        input_value = tune_pseudo_random_bit_sequence() ? max_input : -max_input;
        input_value /= state->battery_voltage;
      }

      // Output
      {
        left = state->encoder_left;
        right = state->encoder_right;

        if (is_left)
        {
          output[i] = (double)(int32_t)(left - previous_position);
          previous_position = left;
        }
        else
        {
          output[i] = (double)(int32_t)(right - previous_position);
          previous_position = right;
        }
      }

      // Input
      {
        if (is_left)
        {
          motor_set_velocity(input_value, 0);
        }
        else
        {
          motor_set_velocity(0, input_value);
        }
        input[i] = input_value;
      }

      i++;
      if (i >= n)
      {
        break;
      }
    }
  }

  // Stop motor
  motor_set_velocity(0, 0);
  motor_enable(false);
}

static void tune_collect()
{
  // System identification with ARX model
  print("Tuning PID...");

#define N 10000
#define DT_NS 1000000 // 1ms
#define ARX_ORDER 2

  double u[N] = {0};
  double v[N] = {0};

  // Collect motor response of left motor
  tune_collect_motor_response(u, v, N, DT_NS, true);

  // Print results for left motor
  printf("----------\n");
  fflush(stdout);
  for (uint32_t i = 0; i < N; i++)
  {
    printf("%f, %f\n", u[i], v[i]);
    fflush(stdout);
  }

  printf("----------\n");
  fflush(stdout);

  // Collect motor response of right motor
  tune_collect_motor_response(u, v, N, DT_NS, false);

  // Print results for right motor
  fflush(stdout);
  for (uint32_t i = 0; i < N; i++)
  {
    printf("%f, %f\n", u[i], v[i]);
    fflush(stdout);
  }
  printf("----------\n");
  fflush(stdout);
}
