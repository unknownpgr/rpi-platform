#include <services/line.h>

#include <state.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define NUM_SENSORS 16
#define NUM_POS_CANDIDATES 1000

static double sensor_positions[NUM_SENSORS];
static double candidate_positions[NUM_POS_CANDIDATES];

static double line_mu(double distance)
{
  double mu = 1 - 3 * ABS(distance);
  if (mu < 0)
  {
    mu = 0;
  }
  return mu;
}

static void line_setup()
{
  for (int i = 0; i < NUM_POS_CANDIDATES; i++)
  {
    candidate_positions[i] = i * 2.0 / (NUM_POS_CANDIDATES - 1) - 1.0;
  }
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    sensor_positions[i] = i * 2.0 / (NUM_SENSORS - 1) - 1.0;
  }
}

static void line_loop_weighted_sum()
{
  double weighted_sum = 0;
  double weight_sum = 0;
  double prev_position = state->position;

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    double weight = state->sensor_data[i];
    if (ABS(sensor_positions[i] - prev_position) > 0.3)
    {
      weight = 0;
    }
    weighted_sum += weight * sensor_positions[i];
    weight_sum += weight;
  }

  if (weight_sum == 0)
  {
    state->position = prev_position;
    return;
  }

  state->position = weighted_sum / weight_sum;
}

static void line_loop_bayesian()
{
  double optimal_likelihood = 999999999;
  double optimal_position = 0;
  double prev_position = state->position;

  for (int i = 0; i < NUM_POS_CANDIDATES; i++)
  {
    double candidate_position = candidate_positions[i];
    double likelihood = 0;

    // Set the prior
    double tmp = candidate_position - prev_position;
    likelihood += 4 * tmp * tmp;

    // Add evidence
    for (int j = 0; j < NUM_SENSORS; j++)
    {
      double tmp = state->sensor_data[j] - line_mu(candidate_position - sensor_positions[j]);
      // tmp is the difference between the predicted value and the actual value.
      // Therefore, we need to find the place that minimizes this value.
      likelihood += tmp * tmp;
    }

    // Check if this is the best position
    if (likelihood < optimal_likelihood)
    {
      optimal_likelihood = likelihood;
      optimal_position = candidate_position;
    }
  }

  state->position = optimal_position;
}

em_service_t service_line = {
    .state_mask = EM_STATE_ALL,
    .setup = line_setup,
    .loop = line_loop_weighted_sum,
    .teardown = NULL,
};
