#include <timer.h>
#include <time.h>

static uint32_t timer_s = 0;
static uint32_t timer_ns = 0;
static uint32_t timer_prev_ns = 0;

bool timer_init()
{
    timer_update();
    return true;
}

static inline void __timer_update()
{
    // Get current time
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    // Update timer_ns
    timer_ns = ts.tv_nsec;

    // Calculate time since last update
    if (timer_ns < timer_prev_ns)
    {
        timer_s++;
    }
    timer_prev_ns = timer_ns;
}

void timer_update()
{
    __timer_update();
}

void timer_sleep_ns(uint32_t ns)
{
    uint32_t start_time = timer_get_ns();
    while (DIFF(timer_get_ns(), start_time) < ns)
        ;
}

uint32_t timer_get_s()
{
    __timer_update();
    return timer_s;
}

uint32_t timer_get_ns()
{
    __timer_update();
    return timer_ns;
}