#include <music-service.h>

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <dev.h>
#include <log.h>
#include <motor.h>
#include <timer.h>

void music_play(char *music_file_path)
{
    int fd = open(music_file_path, O_RDONLY);
    if (fd == -1)
    {
        print("Failed to open music file");
        return;
    }

    uint32_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    uint8_t *music_data = (uint8_t *)malloc(file_size);
    if (read(fd, music_data, file_size) != file_size)
    {
        print("Failed to read music file");
        close(fd);
        return;
    }
    close(fd);

    /**
     * Sample rate = 44.1kHz
     * :. interval = 1 / 44.1kHz = 22.6757us
     */

    motor_enable(true);
    float volume_gain = 0.98f;
    float irr_gain = 0.50f;
    float epsilon = 0.001;

    float filtered = 0;
    uint32_t i = 0;
    uint32_t last_time = timer_get_ns();

    while (true)
    {
        uint32_t current_time = timer_get_ns();
        uint32_t elapsed_time = DIFF(current_time, last_time);
        if (elapsed_time >= 22676)
        {
            last_time = current_time;

            float current_value = ((music_data[i]) / 255.f);                                 // Convert uint8_t to float
            filtered = current_value - irr_gain * filtered + (1 - irr_gain) * current_value; // Apply IIR filter
            float output = volume_gain * filtered + epsilon;                                 // Apply gain and bias

            // Clip output
            if (output > 0.9f)
                output = 0.9f;

            motor_set_velocity(output, output);

            i++;
            if (i >= file_size)
            {
                break;
            }
        }
    }

    print("Done");
}