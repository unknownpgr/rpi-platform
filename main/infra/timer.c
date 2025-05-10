#include <ports/timer.h>

#include <time.h>

bool timer_init()
{
    return true;
}

void timer_sleep_ns(uint32_t ns)
{
    uint32_t start_time = timer_get_ns();
    while (DIFF(timer_get_ns(), start_time) < ns)
        ;
}

uint32_t timer_get_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_nsec;
}

void loop_init(loop_t *loop, uint32_t interval_ns)
{
    loop->interval_ns = interval_ns;
    loop->last_time_ns = timer_get_ns();
}

bool loop_update(loop_t *loop, uint32_t *dt_ns)
{
    uint32_t current_time = timer_get_ns();
    *dt_ns = DIFF(current_time, loop->last_time_ns);
    if (*dt_ns >= loop->interval_ns)
    {
        loop->last_time_ns = current_time;
        return true;
    }
    return false;
}
