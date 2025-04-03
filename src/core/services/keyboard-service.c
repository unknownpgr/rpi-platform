#include "keyboard-service.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

void keyboard_init()
{
    struct termios termios_setting;
    tcgetattr(STDIN_FILENO, &termios_setting);
    termios_setting.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    termios_setting.c_cc[VMIN] = 1;              // Minimum number of characters to read
    termios_setting.c_cc[VTIME] = 0;             // Disable read timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &termios_setting);

    // Non-blocking input
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
}

uint16_t keyboard_get_key()
{
    char c;

    while (1)
    {
        c = getchar();
        if (c == 255)
        {
            return KEY_NONE;
        }

        if (c == 27)
        {
            if (getchar() == '[')
            {
                switch (getchar())
                {
                case 'A':
                    return KEY_UP;
                case 'B':
                    return KEY_DOWN;
                case 'C':
                    return KEY_RIGHT;
                case 'D':
                    return KEY_LEFT;
                default:
                    return 0;
                }
            }
        }
        else
        {
            return c;
        }
    }
}

void keyboard_wait_any_key()
{
    char c;
    while (1)
    {
        c = keyboard_get_key();
        if (c != KEY_NONE)
        {
            return;
        }
    }
}