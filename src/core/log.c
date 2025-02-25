#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <log.h>

uint32_t start_time = 0;

void print(const char *format, ...)
{
    if (start_time == 0)
    {
        start_time = time(NULL);
    }

    char buf[1000];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    time_t now = time(NULL);
    printf("[%02ld] %s", now - start_time, buf);
    fflush(stdout);
}
