#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include <log.h>
#include <timer.h>

float start_time_s = -1;

void print(const char *format, ...)
{
    // Get current seconds in float
    float s = timer_get_ns() / 1000000000.f + timer_get_s();

    // Initialize start_time_s if it is not initialized
    if (start_time_s < 0)
    {
        // Skip initialization if timer is not initialized
        if (s > 0)
        {
            start_time_s = s;
        }
    }

    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    printf("[%07.3f] %s\r\n", (s - start_time_s), buf);
    fflush(stdout);
}

void error(const char *format, ...)
{
    printf("\033[31m");
    print(format);
    printf("\033[0m");
}

void warning(const char *format, ...)
{
    printf("\033[33m");
    print(format);
    printf("\033[0m");
}

void clear()
{
    printf("\033[H\033[J");
}