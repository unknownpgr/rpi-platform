#include <timer.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t timer_s = 0;
static uint32_t timer_prev_ns = 0;

static inline uint32_t __get_time_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_nsec;
}

static inline void __timer_update()
{
    // Lock mutex to ensure thread safety
    pthread_mutex_lock(&lock);

    uint32_t timer_ns = __get_time_ns();

    // Calculate time since last update
    if (timer_ns < timer_prev_ns)
    {
        timer_s++;
    }
    timer_prev_ns = timer_ns;

    // Unlock mutex
    pthread_mutex_unlock(&lock);
}

bool timer_init()
{
    return true;
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
    return timer_s;
}

uint32_t timer_get_ns()
{
    return __get_time_ns();
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
