#include <stdio.h>
#include <unistd.h>
#include <dev.h>

int main()
{
    printf("Hello, World!\n");

    if (!dev_init())
    {
        printf("Failed to initialize device\n");
        return 1;
    }

    printf("Device initialized\n");

    dev_gpio_set_mode(18, GPIO_FSEL_OUT);
    dev_gpio_set_pull(18, GPIO_PUD_DOWN);

    for (int i = 0; i < 10; i++)
    {
        printf("[%d] Blinking\n", i);
        fflush(stdout);

        dev_gpio_set(1 << 18);
        sleep(1);
        dev_gpio_clear(1 << 18);
        sleep(1);
    }

    return 0;
}