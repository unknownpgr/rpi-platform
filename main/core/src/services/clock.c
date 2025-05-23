#include <services/clock.h>

#include <pthread.h>

#include <state.h>
#include <ports/timer.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t timer_s = 0;
static uint32_t timer_prev_ns = 0;

static void clock_setup()
{
  timer_s = 0;
  timer_prev_ns = 0;
}

static void clock_loop()
{
  // Lock mutex to ensure thread safety
  pthread_mutex_lock(&lock);

  uint32_t timer_ns = timer_get_ns();

  // Calculate time since last update
  if (timer_ns < timer_prev_ns)
  {
    timer_s++;
  }
  timer_prev_ns = timer_ns;

  // Unlock mutex
  pthread_mutex_unlock(&lock);
}

uint32_t clock_get_s()
{
  return timer_s;
}

em_service_t service_clock = {
    .state_mask = EM_STATE_ALL,
    .setup = clock_setup,
    .loop = clock_loop,
    .teardown = NULL,
};
