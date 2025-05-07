#include <em.h>
#include <state.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <ports/timer.h>

/*
 * GP: global-previous
 * GC: global-current
 * LP: local-previous
 * LC: local-current
 *
 * GP GC LP LC
 * A  A  A  A  : Loop
 * A  B  A  A  : Teardown - teardown referring to the global context - only this phase refers to the global context
 * A  B  A  B  : Intermission - run services that are both in the current and previous state
 * B  B  A  B  : Setup - setup referring to the local context
 * B  B  B  B  : Loop
 *
 * Therefore the initial state of the global context must be IDLE, IDLE,
 * and the initial state of the local context must be HALT, IDLE.
 */

void em_init_context(em_context_t *context)
{
  context->curr_state = EM_STATE_IDLE;
  context->prev_state = EM_STATE_IDLE;
  context->num_running_contexts = 0;
  pthread_mutex_init(&context->mutex, NULL);
}

void em_init_local_context(em_local_context_t *local_context, em_context_t *context)
{
  local_context->context = context;
  local_context->prev_state = EM_STATE_HALT;
  local_context->curr_state = EM_STATE_IDLE;
  local_context->num_services = 0;
  for (uint32_t i = 0; i < EM_MAX_EXECUTION_CONTEXTS; i++)
  {
    local_context->services[i] = (em_service_t){0};
  }
}

void em_add_service(em_local_context_t *local_context, em_service_t *service)
{
  if (local_context->num_services >= EM_MAX_EXECUTION_CONTEXTS)
  {
    return;
  }
  local_context->services[local_context->num_services++] = *service;
}

#define PHASE_SETUP 0x01
#define PHASE_LOOP 0x00
#define PHASE_TEARDOWN 0x02
#define PHASE_INTERMISSION 0x03

static inline uint8_t em_get_phase(em_local_context_t *local_context)
{
  em_context_t *context = local_context->context;
  uint8_t is_global_changed = context->prev_state != context->curr_state ? 2 : 0;
  uint8_t is_local_changed = local_context->prev_state != local_context->curr_state ? 1 : 0;
  return is_global_changed | is_local_changed;
}

static inline void em_counter_up(em_local_context_t *local_context)
{
  em_context_t *context = local_context->context;
  pthread_mutex_lock(&context->mutex);
  context->num_running_contexts++;
  pthread_mutex_unlock(&context->mutex);
}

static inline bool em_counter_down(em_local_context_t *local_context)
{
  em_context_t *context = local_context->context;
  pthread_mutex_lock(&context->mutex);
  context->num_running_contexts--;
  bool is_last = context->num_running_contexts == 0;
  pthread_mutex_unlock(&context->mutex);
  return is_last;
}

#define CHECK_STATE(service, state) ((service)->state_mask & (state))

bool em_update(em_local_context_t *local_context)
{
  uint8_t phase = em_get_phase(local_context);
  switch (phase)
  {
  case PHASE_SETUP:
  {
    for (uint32_t i = 0; i < local_context->num_services; i++)
    {
      em_service_t *service = &local_context->services[i];

      // Setup if the service is not in the previous state and in the current state
      if (!CHECK_STATE(service, local_context->prev_state) && CHECK_STATE(service, local_context->curr_state) && service->setup != NULL)
      {
        service->setup();
      }
    }
    local_context->prev_state = local_context->curr_state; // Move to loop state
    em_counter_up(local_context);
  }
  break;
  case PHASE_LOOP:
  {
    for (uint32_t i = 0; i < local_context->num_services; i++)
    {
      em_service_t *service = &local_context->services[i];
      // Loop if the service is in the current state
      if (CHECK_STATE(service, local_context->curr_state) && service->loop != NULL)
      {
        service->loop();
      }
    }
  }
  break;
  case PHASE_TEARDOWN:
  {
    em_context_t *context = local_context->context;
    for (uint32_t i = 0; i < local_context->num_services; i++)
    {
      em_service_t *service = &local_context->services[i];
      // Teardown if the service is in the global previous state and not in the global current state
      if (CHECK_STATE(service, context->prev_state) && !CHECK_STATE(service, context->curr_state) && service->teardown != NULL)
      {
        service->teardown();
      }
    }
    local_context->prev_state = local_context->curr_state; // Move to intermission state
    if (em_counter_down(local_context))
    {
      context->prev_state = context->curr_state;
    }

    // Break loop if the global context is HALT
    if (context->curr_state == EM_STATE_HALT)
    {
      return false;
    }
  }
  break;
  case PHASE_INTERMISSION:
  {
    for (uint32_t i = 0; i < local_context->num_services; i++)
    {
      em_service_t *service = &local_context->services[i];
      if (CHECK_STATE(service, local_context->curr_state) && CHECK_STATE(service, local_context->prev_state) && service->loop != NULL)
      {
        service->loop();
      }
    }
  }
  break;
  }
  return true;
}