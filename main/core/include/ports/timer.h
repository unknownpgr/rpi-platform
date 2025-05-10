#pragma once

#include <stdint.h>
#include <stdbool.h>

#define TIMER_NS_MAX 1000000000
#define DIFF(to, from) ((TIMER_NS_MAX + (to) - (from)) % TIMER_NS_MAX)
#define ACCUM(from, diff) (((from) + (diff)) % TIMER_NS_MAX)

bool timer_init();
void timer_sleep_ns(uint32_t ns);
uint32_t timer_get_ns();

// Loop-related

typedef struct
{
    uint32_t interval_ns;
    uint32_t last_time_ns;
} loop_t;

void loop_init(loop_t *loop, uint32_t interval_ns);
bool loop_update(loop_t *loop, uint32_t *dt_ns);