#define _GNU_SOURCE
#include <main.h>
#include <state.h>
#include <em.h>

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

#include <ports/log.h>
#include <ports/timer.h>
#include <ports/motor.h>

#include <services/encoder.h>
#include <services/clock.h>
#include <services/imu.h>
#include <services/drive.h>
#include <services/music.h>
#include <services/vsense.h>
#include <services/sensor.h>

void pin_thread_to_cpu(int cpu)
{
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_t thread = pthread_self();
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#else
    print("Warning: Pinning thread to CPU is not supported on this platform");
#endif
}

#define SHM_NAME "/state"
#define SHM_SIZE 4096

state_t *state;

em_context_t em_context;

em_local_context_t em_local_1;
em_local_context_t em_local_2;
em_local_context_t em_local_3;

void thread_1(void *_)
{
    while (em_update(&em_local_1))
    {
        usleep(10000); // Sleep for 10ms
    }
    print("Thread 1 finished");
}

void thread_2(void *_)
{
    pin_thread_to_cpu(2);
    EM_LOOP(&em_local_2);
    print("Thread 2 finished");
}

void thread_3(void *_)
{
    pin_thread_to_cpu(3);
    EM_LOOP(&em_local_3);
    print("Thread 3 finished");
}

void init_em()
{
    em_init_context(&em_context);

    em_init_local_context(&em_local_1, &em_context); // Timer
    em_init_local_context(&em_local_2, &em_context); // Encoder
    em_init_local_context(&em_local_3, &em_context); // Sensor & Drive

    em_add_service(&em_local_1, &service_clock);
    em_add_service(&em_local_2, &service_encoder);
    em_add_service(&em_local_3, &service_sensor);
    em_add_service(&em_local_3, &service_sensor_low);
    em_add_service(&em_local_3, &service_sensor_high);
    em_add_service(&em_local_3, &service_vsense);
}

int init_state()
{
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        error("Error creating shared memory");
        return -1;
    }

    // Truncate shared memory to size
    ftruncate(shm_fd, SHM_SIZE);

    // Map shared memory to process
    state = (state_t *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED)
    {
        error("Error mapping shared memory");
        return -1;
    }

    // Initialize state
    memset(state, 0, SHM_SIZE);

    return 0;
}

int init_ui_server()
{
    int pipe_read[2]; // [0] is read, [1] is write
    int pipe_write[2];
    pipe(pipe_read);
    pipe(pipe_write);
    fcntl(pipe_read[0], F_SETFL, 0);
    fcntl(pipe_write[0], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid == 0)
    // Child process
    {
        char fd_str[10];
        sprintf(fd_str, "%d", pipe_read[1]);

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
    // Parent processs
    {
        close(pipe_read[1]);
        close(pipe_write[0]);

        // Redirect stdout to pipe_write[1]
        dup2(pipe_write[1], 1);
    }
    return 0;
}

int application_start()
{
    print("Program started");

    int error;

    init_em();

    error = init_state();
    if (error != 0)
    {
        print("Error initializing state");
        return error;
    }
    print("State initialized");

    error = init_ui_server();
    if (error != 0)
    {
        print("Error initializing UI server");
        return error;
    }
    print("UI server initialized");

    char buffer[1024];
    state_print_offsets(state, buffer);
    print(buffer);

    // Start threads
    pthread_t threads[3];
    error = pthread_create(&threads[0], NULL, (void *)thread_1, NULL);
    if (error != 0)
    {
        print("Error creating thread 1: %d", error);
        return error;
    }

    error = pthread_create(&threads[1], NULL, (void *)thread_2, NULL);
    if (error != 0)
    {
        print("Error creating thread 2: %d", error);
        return error;
    }

    error = pthread_create(&threads[2], NULL, (void *)thread_3, NULL);
    if (error != 0)
    {
        print("Error creating thread 3: %d", error);
        return error;
    }

    // // Receive message from UI server
    // char buffer[1024];
    // uint16_t len = 0;
    // while (state->state != STATE_EXIT)
    // {
    //     char c;
    //     ssize_t bytes_read = read(pipe_read[0], &c, 1);
    //     if (bytes_read == 0)
    //     {
    //         continue;
    //     }

    //     if (c == '\n')
    //     {
    //         buffer[len] = '\0';
    //     }
    //     else
    //     {
    //         buffer[len++] = c;
    //         continue;
    //     }

    //     if (strcmp(buffer, "quit") == 0)
    //     {
    //         print("Quitting...");
    //         state->state = STATE_EXIT;
    //     }

    //     else if (strcmp(buffer, "cali_low") == 0)
    //     {
    //         print("Calibrating low");
    //         state->state = STATE_CALI_LOW;

    //         // Reset low values
    //         for (int i = 0; i < 16; i++)
    //         {
    //             state->sensor_low[i] = 0;
    //         }
    //     }

    //     else if (strcmp(buffer, "cali_high") == 0)
    //     {
    //         print("Calibrating high");
    //         motor_enable(false);
    //         state->state = STATE_CALI_HIGH;

    //         // Reset high values
    //         for (int i = 0; i < 16; i++)
    //         {
    //             state->sensor_high[i] = 0;
    //         }
    //     }

    //     else if (strcmp(buffer, "cali_save") == 0)
    //     {
    //         print("Saving calibration");
    //         state->state = STATE_IDLE;
    //         sensor_save_calibration();
    //     }

    //     else if (strcmp(buffer, "drive") == 0)
    //     {
    //         print("Driving");
    //         drive_init();
    //         state->state = STATE_DRIVE;
    //     }

    //     else if (strcmp(buffer, "idle") == 0)
    //     {
    //         print("Idling");
    //         motor_enable(false);
    //         state->speed = 0.0;
    //         state->state = STATE_IDLE;
    //     }

    //     else
    //     {
    //         print("Unknown command: %s", buffer);
    //     }
    //     len = 0;
    // }

    // Join threads
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    print("All threads joined");

    return 0;
}