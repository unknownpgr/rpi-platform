#include <stdio.h>
#include <unistd.h>
#include <dev.h>
#include <motor.h>
#include <log.h>
#include <math.h>

int main()
{
    print("Program started\n");

    if (!dev_init())
    {
        print("Failed to initialize device\n");
        return 1;
    }
    print("Device initialized\n");

    if (!motor_init())
    {
        print("Failed to initialize motor\n");
        return 1;
    }
    print("Motor initialized\n");

    motor_enable(true);
    print("Motor enabled\n");

    for (int i = 0; i < 1000; i++)
    {
        float v = sin(i / 100.0);
        print("Velocity: %+1.4f\n", v);
        motor_set_velocity(v, v);
        usleep(10000); // 100ms
    }

    motor_enable(false);
    print("Motor disabled\n");

    return 0;
}