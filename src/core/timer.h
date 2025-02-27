#pragma once

#include <stdint.h>
#include <stdbool.h>

#define TIMER_NS_MAX 1000000000

bool timer_init();
void timer_update();
uint32_t timer_get_s();
uint32_t timer_get_ns();