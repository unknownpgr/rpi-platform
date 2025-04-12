#pragma once

#include <stdbool.h>
#include <stdint.h>

bool encoder_init();
void encoder_update();
void encoder_get_counts(uint32_t *left, uint32_t *right);