#pragma once

#include <stdint.h>

#define EM_MAX_EXECUTION_CONTEXTS 32

#define STATE_IDLE 0x01
#define STATE_HALT 0x00

typedef uint32_t state_t;
typedef void (*function_void_t)(void);
typedef void (*function_loop_t)(uint32_t dt_ns);

typedef struct
{
  uint32_t interval_ns;
  state_t state_mask;
  function_void_t setup;
  function_loop_t loop;
  function_void_t teardown;
} execution_configuration_t;

typedef struct
{
  state_t state;
  state_t previous_state;
  uint32_t num_contexts;
  execution_context_t contexts[EM_MAX_EXECUTION_CONTEXTS];
} execution_manager_t;

void em_init(execution_manager_t *em);
void em_add_context(execution_manager_t *em, execution_context_t *context);
void em_update(execution_manager_t *em);
