#include <knob-service.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <encoder.h>
#include <motor.h>
#include <timer.h>
#include <log.h>

typedef struct
{
    uint32_t interval_ns;
    uint32_t last_time_ns;
} interval_t;

interval_t create_interval(uint32_t interval_ns)
{
    interval_t interval = {
        .interval_ns = interval_ns,
        .last_time_ns = timer_get_ns(),
    };
    return interval;
}

#define ON(interval, code)                                                                             \
    if ((TIMER_NS_MAX + timer_get_ns() - interval.last_time_ns) % TIMER_NS_MAX > interval.interval_ns) \
    {                                                                                                  \
        interval.last_time_ns = (interval.last_time_ns + interval.interval_ns) % TIMER_NS_MAX;         \
        {code};                                                                                        \
    }

void knob_test()
{
    int counter = 0;
    print("Start counter");
    interval_t interval = create_interval(10000000);
    while (true)
    {
        ON(interval, {
            counter++;
            if (counter >= 1000)
                break;
        })
    }
    print("Counter: %d", counter);
}