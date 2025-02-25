#include <stdio.h>
#include <unistd.h>
#include <dev.h>
#include <motor.h>

int main()
{
    printf("Hello, World!\n");

    if (!dev_init())
    {
        printf("Failed to initialize device\n");
        return 1;
    }

    if (!motor_init())
    {
        printf("Failed to initialize motor\n");
        return 1;
    }

    motor_enable(true);
    motor_set_velocity(1.0, 1.0);
    sleep(1);
    motor_set_velocity(-1.0, -1.0);
    sleep(1);
    motor_enable(false);

    return 0;
}