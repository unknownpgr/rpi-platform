#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define EM_MAX_EXECUTION_CONTEXTS 32

#define EM_LOOP(em_local_context)     \
  while (em_update(em_local_context)) \
    ;

typedef uint32_t em_state_t;
typedef void (*function_t)(void);

typedef struct
{
  em_state_t curr_state;
  em_state_t prev_state;
  uint32_t num_running_contexts;
  pthread_mutex_t mutex;
} em_context_t;

typedef struct
{
  em_state_t state_mask;
  function_t setup;
  function_t loop;
  function_t teardown;
} em_service_t;

typedef struct
{
  em_context_t *context;
  em_state_t curr_state;
  em_state_t prev_state;
  uint32_t num_services;
  em_service_t services[EM_MAX_EXECUTION_CONTEXTS];
} em_local_context_t;

void em_init_context(em_context_t *context);
void em_init_local_context(em_local_context_t *local_context, em_context_t *context);
void em_add_service(em_local_context_t *local_context, em_service_t *service);
bool em_update(em_local_context_t *local_context);
