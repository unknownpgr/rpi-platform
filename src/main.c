#include <stdio.h>
#include <unistd.h>
#include <dev.h>
#include <motor.h>
#include <log.h>
#include <math.h>
#include <encoder.h>

int main()
{
    print("Program started");

    if (!dev_init())
    {
        print("Failed to initialize device");
        return 1;
    }
    print("Device initialized");

    if (!motor_init())
    {
        print("Failed to initialize motor");
        return 1;
    }
    print("Motor initialized");

    if (!encoder_init())
    {
        print("Failed to initialize encoder");
        return 1;
    }
    print("Encoder initialized");

    motor_enable(true);
    print("Motor enabled");

    uint32_t target_position = 0;
    uint32_t current_position = 0;
    float gain = 0.0001;

    while (true)
    {
        encoder_update();
        encoder_get_counts(&current_position, &current_position);

        int32_t diff = target_position - current_position;
        float speed = -gain * diff;
        if (speed > 1)
            speed = 1;
        if (speed < -1)
            speed = -1;
        motor_set_velocity(0, speed);
    }

    return 0;
}