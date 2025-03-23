#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct imu_data_t
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} imu_data_t;

typedef struct imu_geomagnetic_t
{
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t r;
} imu_geomagnetic_t;

void imu_init();
void imu_read(imu_data_t *data);
void imu_read_geomagnetic(imu_geomagnetic_t *data);