#define _GNU_SOURCE
#include <main.h>

#include <math.h>
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>

#include <log.h>
#include <timer.h>
#include <motor.h>
#include <linalg.h>
#include <encoder.h>

#include <imu-service.h>
#include <drive-service.h>
#include <music-service.h>
#include <vsense-service.h>
#include <sensor-service.h>
#include <keyboard-service.h>

void pin_thread_to_cpu(int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

#define STATE_EXIT 0x00
#define STATE_IDLE 0x01
#define STATE_CALI 0x02
#define STATE_DRIV 0x03

#define SHM_NAME "/state"
#define SHM_SIZE 4096

typedef struct
{
    uint8_t state;
    sensor_state_t sensor_state;
    drive_state_t drive_state;
    int encoder_left;
    int encoder_right;
} state_t;

state_t *state;

void thread_timer(void *_)
{
    print("Timer thread started");
    while (state->state != STATE_EXIT)
    {
        usleep(10000); // Sleep for 10ms
        timer_update();
    }
}

void thread_encoder(void *_)
{
    print("Encoder thread started");
    pin_thread_to_cpu(2);

    loop_t loop_encoder;
    uint32_t dt_ns;
    loop_init(&loop_encoder, 1000); // 1us
    while (state->state != STATE_EXIT)
    {
        if (loop_update(&loop_encoder, &dt_ns))
        {
            encoder_update();
            encoder_get_counts(&state->encoder_left, &state->encoder_right);
        }
    }
}

void thread_drive(void *_)
{
    pin_thread_to_cpu(3);

    while (state->state != STATE_EXIT)
    {
        // TODO: SPI is terribly slow. We need to optimize this.
        // About 10848 loops per second. (~11kHz)
        // Much lower then I expected. (~100kHz)

        // Read sensor values

        switch (state->state)
        {
        case STATE_IDLE:
            break;

        case STATE_CALI:
            sensor_loop();
            break;

        case STATE_DRIV:
            sensor_loop();
            drive_loop();
            break;

        default:
            break;
        }
    }
}

int application_start()
{
    print("Program started");

    // Initialize state
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        error("Error creating shared memory");
        return -1;
    }
    ftruncate(shm_fd, SHM_SIZE);
    state = (state_t *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED)
    {
        error("Error mapping shared memory");
        return -1;
    }
    memset(state, 0, SHM_SIZE);
    state->state = STATE_IDLE;

    // Start UI server
    int pipe_read[2]; // [0] is read, [1] is write
    int pipe_write[2];
    pipe(pipe_read);
    pipe(pipe_write);
    fcntl(pipe_read[0], F_SETFL, 0);
    fcntl(pipe_write[0], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid == 0)
    {
        char fd_str[10];
        sprintf(fd_str, "%d", pipe_read[1]);

        // Child process
        close(pipe_read[0]);
        close(pipe_write[1]);

        // Redirect stdin to pipe_write[0]
        dup2(pipe_write[0], 0);

        execlp("node", "node", "../src-broaker/app.js", fd_str, (char *)NULL);
        print("Failed to start UI server");
        exit(0);
    }
    else if (pid < 0)
    {
        print("Error starting UI server");
        return -1;
    }
    else
    {
        // Parent process
        close(pipe_read[1]);
        close(pipe_write[0]);

        // Redirect stdout to pipe_write[1]
        dup2(pipe_write[1], 1);
    }

    // Print all address offsets for state
    uint64_t base = (uint64_t)state;

    printf("state.state:%ld:uint8_t\n", (uint64_t)(&state->state) - base);

    // Sensor state
    printf("state.sensor_state:%ld:sensor_state_t\n", (uint64_t)(&state->sensor_state) - base);
    printf("state.sensor_state.state:%ld:uint8_t\n", (uint64_t)(&state->sensor_state.state) - base);
    printf("state.sensor_state.raw_data:%ld:uint16_t[16]\n", (uint64_t)(&state->sensor_state.raw_data) - base);
    printf("state.sensor_state.sensor_data:%ld:double[16]\n", (uint64_t)(&state->sensor_state.sensor_data) - base);
    printf("state.sensor_state.calibration:%ld:calibration_t\n", (uint64_t)(&state->sensor_state.calibration) - base);
    printf("state.sensor_state.calibration.sensor_low:%ld:uint16_t[16]\n", (uint64_t)(&state->sensor_state.calibration.sensor_low) - base);
    printf("state.sensor_state.calibration.sensor_high:%ld:uint16_t[16]\n", (uint64_t)(&state->sensor_state.calibration.sensor_high) - base);
    printf("state.sensor_state.is_calibrated:%ld:bool\n", (uint64_t)(&state->sensor_state.is_calibrated) - base);

    // Drive state
    printf("state.drive_state:%ld:drive_state_t\n", (uint64_t)(&state->drive_state) - base);
    printf("state.drive_state.pid_left:%ld:pid_t\n", (uint64_t)(&state->drive_state.pid_left) - base);
    printf("state.drive_state.pid_right:%ld:pid_t\n", (uint64_t)(&state->drive_state.pid_right) - base);
    printf("state.drive_state.left:%ld:int32_t\n", (uint64_t)(&state->drive_state.left) - base);
    printf("state.drive_state.right:%ld:int32_t\n", (uint64_t)(&state->drive_state.right) - base);
    printf("state.drive_state.left_prev:%ld:int32_t\n", (uint64_t)(&state->drive_state.left_prev) - base);
    printf("state.drive_state.right_prev:%ld:int32_t\n", (uint64_t)(&state->drive_state.right_prev) - base);
    printf("state.drive_state.loop_motor:%ld:loop_t\n", (uint64_t)(&state->drive_state.loop_motor) - base);
    printf("state.drive_state.loop_vsense:%ld:loop_t\n", (uint64_t)(&state->drive_state.loop_vsense) - base);
    printf("state.drive_state.position:%ld:double\n", (uint64_t)(&state->drive_state.position) - base);
    printf("state.drive_state.battery_voltage:%ld:double\n", (uint64_t)(&state->drive_state.battery_voltage) - base);

    // Encoder state
    printf("state.encoder_left:%ld:int32_t\n", (uint64_t)(&state->encoder_left) - base);
    printf("state.encoder_right:%ld:int32_t\n", (uint64_t)(&state->encoder_right) - base);
    printf("--------\n");

    print("UI server started");

    // Initialize peripherals
    imu_init();
    motor_init();
    vsense_init();
    encoder_init();
    sensor_init(&state->sensor_state);
    timer_init();

    print("Peripherals initialized");

    int ret;

    // Start timer thread
    pthread_t timer_thread;
    ret = pthread_create(&timer_thread, NULL, (void *)thread_timer, NULL);
    if (ret != 0)
    {
        print("Error creating timer thread: %d", ret);
        return -1;
    }

    // Start encoder thread
    pthread_t encoder_thread;
    ret = pthread_create(&encoder_thread, NULL, (void *)thread_encoder, NULL);
    if (ret != 0)
    {
        print("Error creating encoder thread: %d", ret);
        return -1;
    }

    // Start drive thread
    pthread_t drive_thread;
    ret = pthread_create(&drive_thread, NULL, (void *)thread_drive, NULL);
    if (ret != 0)
    {
        print("Error creating drive thread: %d", ret);
        return -1;
    }

    // Receive message from UI server
    char buffer[1024];
    uint16_t len = 0;
    while (state->state != STATE_EXIT)
    {
        char c;
        ssize_t bytes_read = read(pipe_read[0], &c, 1);
        if (bytes_read == 0)
        {
            continue;
        }

        if (c == '\n')
        {
            buffer[len] = '\0';
        }
        else
        {
            buffer[len++] = c;
            continue;
        }

        if (strcmp(buffer, "quit") == 0)
        {
            print("Quitting...");
            state->state = STATE_EXIT;
        }
        else if (strcmp(buffer, "cali_low") == 0)
        {
            print("Calibrating low");
            state->state = STATE_CALI;
            state->sensor_state.state = STATE_CALI_LOW;

            // Reset low values
            for (int i = 0; i < 16; i++)
            {
                state->sensor_state.calibration.sensor_low[i] = 0;
            }

            state->sensor_state.is_calibrated = true;
        }
        else if (strcmp(buffer, "cali_high") == 0)
        {
            print("Calibrating high");
            state->state = STATE_CALI;
            state->sensor_state.state = STATE_CALI_HIGH;

            // Reset high values
            for (int i = 0; i < 16; i++)
            {
                state->sensor_state.calibration.sensor_high[i] = 0;
            }
            state->sensor_state.is_calibrated = true;
        }
        else if (strcmp(buffer, "cali_save") == 0)
        {
            print("Saving calibration");
            state->state = STATE_IDLE;
            state->sensor_state.state = STATE_READING;
            sensor_save_calibration();
        }
        else if (strcmp(buffer, "drive") == 0)
        {
            print("Driving");
            drive_init(&state->drive_state);
            state->sensor_state.state = STATE_READING;
            state->state = STATE_DRIV;
        }
        else if (strcmp(buffer, "idle") == 0)
        {
            print("Idling");
            state->state = STATE_IDLE;
            state->sensor_state.state = STATE_READING;
        }
        else
        {
            print("Unknown command: %s", buffer);
        }
        len = 0;
    }

    // Join threads
    pthread_join(timer_thread, NULL);
    pthread_join(encoder_thread, NULL);
    pthread_join(drive_thread, NULL);
    print("All threads joined");

    return 0;
}