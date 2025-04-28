#pragma once

#include <stdint.h>
#include <stdbool.h>

bool clock_init();
void clock_update();
uint32_t clock_get_s();
