#include "keyboard-service.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void keyboard_init()
{
    struct termios termios_setting;
    tcgetattr(STDIN_FILENO, &termios_setting);
    termios_setting.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    termios_setting.c_cc[VMIN] = 1;              // Minimum number of characters to read
    termios_setting.c_cc[VTIME] = 0;             // Disable read timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &termios_setting);
}

uint16_t keyboard_get_key()
{
    char c;

    while (1)
    {
        c = getchar();
        if (c == 27)
        {
            if (getchar() == '[')
            {
                switch (getchar())
                {
                case 'A':
                    return KEY_UP;
                    break;
                case 'B':
                    return KEY_DOWN;
                    break;
                case 'C':
                    return KEY_RIGHT;
                    break;
                case 'D':
                    return KEY_LEFT;
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            return c;
        }
    }
}
