#include <main.h>

#include <stdbool.h>
#include <stdint.h>

#include <log.h>
#include <timer.h>
#include <imu-service.h>

uint32_t dt()
{
    static bool is_initialized = false;
    static uint32_t last_time = 0;

    if (is_initialized)
    {
        uint32_t current_time = timer_get_ns();
        uint32_t delta_time = current_time - last_time;
        last_time = current_time;
        return delta_time;
    }
    else
    {
        last_time = timer_get_ns();
        is_initialized = true;
        return 0;
    }
}

int application_start()
{
    print("Program started");
    imu_init();

    while (true)
    {
        imu_data_t data;

        imu_read(&data);

        print("ax: %d, ay: %d, az: %d, gx: %d, gy: %d, gz: %d", data.ax, data.ay, data.az, data.gx, data.gy, data.gz);

        timer_sleep_ns(10000000);
    }

    return 0;
}