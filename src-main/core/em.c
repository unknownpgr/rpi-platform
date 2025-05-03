#include <em.h>

#include <stdbool.h>
#include <stdlib.h>

#include <ports/timer.h>

typedef struct
{
  uint32_t last_time_ns;
  execution_context_t *context;
} execution_context_t;

void em_init(execution_manager_t *em)
{
  em->state = STATE_IDLE;
  em->previous_state = STATE_HALT;
  em->num_contexts = 0;
  for (uint32_t i = 0; i < EM_MAX_EXECUTION_CONTEXTS; i++)
  {
    em->contexts[i].last_time_ns = 0;
    em->contexts[i].context = NULL;
  }
}

void em_add_context(execution_manager_t *em, execution_context_t *context)
{
  if (em->num_contexts >= EM_MAX_EXECUTION_CONTEXTS)
  {
    return;
  }
  em->contexts[em->num_contexts++] = *context;
}

void em_update(execution_manager_t *em)
{
  if (em->previous_state != em->state)
  {
    // Teardown phase
    for (uint32_t i = 0; i < em->num_contexts; i++)
    {
      execution_configuration_t *config = em->contexts[i].context;

      // Teardown if the configuration is in the previous state and not in the new state
      if (config->state_mask & em->previous_state && !(config->state_mask & em->state) && config->teardown != NULL)
      {
        config->teardown();
      }
    }

    // Setup phase
    for (uint32_t i = 0; i < em->num_contexts; i++)
    {
      execution_context_t *context = &em->contexts[i];
      execution_configuration_t *config = context->context;

      // Setup if the configuration is in the new state and not in the previous state
      if (config->state_mask & em->state && !(config->state_mask & em->previous_state) && config->setup != NULL)
      {
        config->setup();
      }

      context->last_time_ns = timer_get_ns();
    }

    em->previous_state = em->state;
  }
  else
  {
    // Loop phase
    for (uint32_t i = 0; i < em->num_contexts; i++)
    {
      execution_context_t *context = &em->contexts[i];
      execution_configuration_t *config = context->context;

      // Loop if the configuration is in the current state
      if (config->state_mask & em->state && config->loop != NULL)
      {
        uint32_t current_time = timer_get_ns();
        uint32_t dt_ns = DIFF(current_time, context->last_time_ns);
        if (dt_ns >= config->interval_ns)
        {
          config->loop(dt_ns);
          context->last_time_ns = current_time;
        }
      }
    }
  }
}