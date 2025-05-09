#include <services/music.h>

#include <state.h>

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <ports/dev.h>
#include <ports/log.h>
#include <ports/motor.h>
#include <ports/timer.h>

float volume_gain = 0.98f;
float irr_gain = 0.5f;
float epsilon = 0.001;

float filtered;
uint32_t i;
uint8_t *music_data = NULL;
uint32_t last_time;
uint32_t file_size;

void music_setup()
{
    // Initialze filter
    filtered = 0;
    i = 0;

    // Open music file
    const char *music_file_path = "../assets/drip.raw";
    int fd = open(music_file_path, O_RDONLY);
    if (fd == -1)
    {
        print("Failed to open music file");
        return;
    }

    // Get file size
    file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    // Allocate memory for music data
    music_data = (uint8_t *)malloc(file_size);

    // Read music data
    if (read(fd, music_data, file_size) != file_size)
    {
        print("Failed to read music file");
        close(fd);
        return;
    }

    // Close music file
    close(fd);

    /**
     * Sample rate = 44.1kHz
     * :. interval = 1 / 44.1kHz = 22.6757us
     */

    // Enable motor
    motor_enable(true);

    uint32_t last_time = timer_get_ns();
}

void music_play()
{
    if (i >= file_size)
    {
        return;
    }

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
    }
}

void music_teardown()
{
    // Disable motor
    motor_set_velocity(0, 0);
    motor_enable(false);

    // Free music data
    if (music_data != NULL)
    {
        free(music_data);
        music_data = NULL;
    }
}

em_service_t service_music = {
    .state_mask = EM_STATE_MUSIC,
    .setup = music_setup,
    .loop = music_play,
    .teardown = music_teardown,
};