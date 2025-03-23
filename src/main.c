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

#include <knob-service.h>
#include <music-service.h>
#include <radio-service.h>
#include <sensor-service.h>
#include <keyboard-service.h>
#include <drive-service.h>
#include <vsense-service.h>
#include <imu-service.h>

void handle_exit()
{
    // Disable motor
    motor_enable(false);

    // Disable GPCLK
    dev_gpclk_enable(0, false);
    dev_gpclk_enable(1, false);

    // Clear all GPIO
    dev_gpio_clear_mask(0xFFFFFFFFFFFFFFFF);

    // Set GPIO mode to input
    for (uint32_t i = 0; i < 28; i++)
    {
        dev_gpio_set_mode(i, GPIO_FSEL_IN);
    }

    // Exit program
    exit(0);
}

void init()
{
    // Configure signal handler
    struct sigaction action;
    action.sa_handler = handle_exit;
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

    // Initialize keyboard
    keyboard_init();
    print("Keyboard initialized");

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
    init();

    imu_init();
    return;
    // Turn gyro / accel on

    printf("--------------------\n");

    while (true)
    {
        imu_data_t data;
        imu_geomagnetic_t geomagnetic;

        imu_read(&data);
        imu_read_geomagnetic(&geomagnetic);

        // printf("ax: %d, ay: %d, az: %d, gx: %d, gy: %d, gz: %d\n", data.ax, data.ay, data.az, data.gx, data.gy, data.gz);
        printf("%d, %d, %d, %d\n", geomagnetic.x, geomagnetic.y, geomagnetic.z, geomagnetic.r);

        timer_sleep_ns(10000000);
    }

    // drive_test();

    // music_play("../assets/love_is_lonely.raw");

    // sensor_init();
    // sensor_calibrate();
    // sensor_test_calibration();

    handle_exit();
    return 0;
}