#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <fcntl.h>

#include <log.h>
#include <dev.h>
#include <motor.h>
#include <encoder.h>
#include <timer.h>

void signal_handler()
{
    // Disable motor
    motor_enable(false);

    // Disable PWM
    dev_pwm_disable(0, 0);
    dev_pwm_disable(0, 1);
    dev_pwm_disable(1, 0);
    dev_pwm_disable(1, 1);

    // Clear all GPIO
    dev_gpio_clear_mask(0xFFFFFFFFFFFFFFFF);

    // Exit program
    exit(0);
}

void init()
{
    // Configure signal handler
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGPIPE, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    // Initialize peripherals
    if (!dev_init())
    {
        print("Failed to initialize device");
        exit(1);
    }
    print("Device initialized");

    // Initialize timer
    if (!timer_init())
    {
        print("Failed to initialize timer");
        exit(1);
    }
    print("Timer initialized");

    // Initialize motor
    if (!motor_init())
    {
        print("Failed to initialize motor");
        exit(1);
    }
    print("Motor initialized");

    // Initialize encoder
    if (!encoder_init())
    {
        print("Failed to initialize encoder");
        exit(1);
    }
    print("Encoder initialized");

    // Set CPU governor to performance
    int fd = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", O_WRONLY);
    if (fd != -1)
    {
        write(fd, "performance", 11);
        close(fd);
    }
    else
    {
        print("Failed to set CPU governor to performance");
        exit(1);
    }
}

int main()
{
    print("Program started");

    // Initialization
    init();

    uint32_t start_ns = timer_get_ns();
    uint32_t ns;
    uint64_t counter = 0;
    while ((uint32_t)((ns = timer_get_ns()) - start_ns) % TIMER_NS_MAX < 100000000)
    {
        counter++;
    }

    print("Counter: %d", counter);
    print("Time: %lld", ns - start_ns);
    print("Frequency: %fMHz", (float)counter / ((float)(ns - start_ns) / 1000.0f));

    return 0;
}