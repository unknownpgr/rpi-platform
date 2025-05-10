#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>

#include <em.h>
#include <state.h>

#include <ports/dev.h>
#include <ports/log.h>
#include <ports/timer.h>
#include <ports/motor.h>

#include <services/imu.h>
#include <services/line.h>
#include <services/clock.h>
#include <services/drive.h>
#include <services/music.h>
#include <services/vsense.h>
#include <services/sensor.h>
#include <services/encoder.h>

#define SHM_NAME "/state"
#define SHM_SIZE 4096

state_t *state;

em_context_t em_context;

em_local_context_t em_local_1;
em_local_context_t em_local_2;
em_local_context_t em_local_3;

static void pin_thread_to_cpu(int cpu)
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

static void thread_1(void *_)
{
    while (em_update(&em_local_1))
    {
        usleep(10000); // Sleep for 10ms
    }
}

static void thread_2(void *_)
{
    pin_thread_to_cpu(2);
    EM_LOOP(&em_local_2);
}

static void thread_3(void *_)
{
    pin_thread_to_cpu(3);
    EM_LOOP(&em_local_3);
}

static void handle_exit()
{
    // Disable motor
    motor_enable(false);

    // Disable GPCLK
    dev_gpclk_enable(0, false);
    dev_gpclk_enable(1, false);

    // Clear all GPIO
    dev_gpio_clear_mask(0xFFFFFFFFFFFFFFFF);

    // Set GPIO mode to input
    for (uint32_t i = 0; i < 28; i++)
    {
        dev_gpio_set_mode(i, GPIO_FSEL_IN);
    }

    // Exit program
    exit(0);
}

static void init_signal()
{
    struct sigaction action;
    action.sa_handler = handle_exit;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGPIPE, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

static void init_ports()
{
    if (!dev_init())
    {
        error("Failed to initialize device");
        exit(1);
    }
    print("Device initialized");

    // Initialize timer
    if (!timer_init())
    {
        error("Failed to initialize timer");
        exit(1);
    }
    print("Timer initialized");

    // Initialize motor
    if (!motor_init())
    {
        error("Failed to initialize motor");
        exit(1);
    }
    print("Motor initialized");
}

static void init_cpu_governor()
{
    int fd = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", O_WRONLY);
    if (fd != -1)
    {
        write(fd, "performance", 11);
        close(fd);
    }
    else
    {
        error("Failed to set CPU governor to performance");
        exit(1);
    }
}

static void init_em()
{
    em_init_context(&em_context);

    em_init_local_context(&em_local_1, &em_context); // Timer
    em_init_local_context(&em_local_2, &em_context); // Encoder
    em_init_local_context(&em_local_3, &em_context); // Sensor & Drive

    em_add_service(&em_local_1, &service_clock);

    em_add_service(&em_local_2, &service_encoder);
    em_add_service(&em_local_2, &service_music);

    em_add_service(&em_local_3, &service_sensor);
    em_add_service(&em_local_3, &service_sensor_low);
    em_add_service(&em_local_3, &service_sensor_high);
    em_add_service(&em_local_3, &service_line);
    em_add_service(&em_local_3, &service_vsense);
    em_add_service(&em_local_3, &service_drive);
}

static void init_state()
{
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        error("Error creating shared memory");
        exit(1);
    }

    // Truncate shared memory to size
    ftruncate(shm_fd, SHM_SIZE);

    // Map shared memory to process
    state = (state_t *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED)
    {
        error("Error mapping shared memory");
        exit(1);
    }

    // Initialize state
    memset(state, 0, SHM_SIZE);

    print("State initialized");
}

static void init_ui_server(int *pipe_miso, int *pipe_simo, pid_t *pid)
{
    // [0] is read, [1] is write
    int pipe_miso[2]; // master in, slave out (read from server)
    int pipe_simo[2]; // slave in, master out (write to server)
    pid_t pid;

    // Create pipes
    pipe(pipe_miso);
    pipe(pipe_simo);

    pid = fork();
    if (pid > 0)
    // Parent processs
    {
        // Close unused pipe ends
        close(pipe_simo[0]); // slave in
        close(pipe_miso[1]); // slave out

        // Set blocking mode for read
        int flags = fcntl(pipe_miso[0], F_GETFL, 0);
        fcntl(pipe_miso[0], F_SETFL, flags & ~O_NONBLOCK);

        // Redirect stdin, stdout, stderr to pipe_miso[0], pipe_simo[1], pipe_simo[1]
        dup2(pipe_miso[0], 0);
        dup2(pipe_simo[1], 1);
        dup2(pipe_simo[1], 2);

        // Print state offsets
        char buffer[1024];
        state_print_offsets(state, buffer);
        print(buffer);

        *pipe_miso = pipe_miso[0];
        *pipe_simo = pipe_simo[1];
        *pid = pid;
    }
    else if (pid == 0)
    // Child process
    {
        // Close unused pipe ends
        close(pipe_miso[0]); // master in
        close(pipe_simo[1]); // master out

        // Set non-blocking mode for read
        int flags = fcntl(pipe_simo[0], F_GETFL, 0);
        fcntl(pipe_simo[0], F_SETFL, flags | O_NONBLOCK);

        char fd_str[10];
        sprintf(fd_str, "%d,%d", pipe_simo[0], pipe_miso[1]); // (read, write)

        // Start UI server
        execlp("node", "node", "../server/app.js", fd_str, (char *)NULL);
        print("Failed to start UI server");
        exit(0);
    }
    else
    // Error
    {
        error("Error starting UI server");
        exit(1);
    }
}

static void init_threads(pthread_t *threads)
{
    if (pthread_create(&threads[0], NULL, (void *)thread_1, NULL) != 0)
    {
        print("Error creating thread 1");
        exit(1);
    }
    if (pthread_create(&threads[1], NULL, (void *)thread_2, NULL) != 0)
    {
        print("Error creating thread 2");
        exit(1);
    }
    if (pthread_create(&threads[2], NULL, (void *)thread_3, NULL) != 0)
    {
        print("Error creating thread 3");
        exit(1);
    }
}

static void handle_message(char *message)
{
    if (strcmp(message, "quit") == 0)
    {
        print("You cannot quit in this version.");
    }
    else if (strcmp(message, "cali_low") == 0)
    {
        em_set_state(&em_context, EM_STATE_CALI_LOW);
    }
    else if (strcmp(message, "cali_high") == 0)
    {
        em_set_state(&em_context, EM_STATE_CALI_HIGH);
    }
    else if (strcmp(message, "cali_save") == 0)
    {
        em_set_state(&em_context, EM_STATE_IDLE);
    }
    else if (strcmp(message, "drive") == 0)
    {
        em_set_state(&em_context, EM_STATE_DRIVE);
    }
    else if (strcmp(message, "idle") == 0)
    {
        em_set_state(&em_context, EM_STATE_IDLE);
    }
    else
    {
        print("Unknown command: %s", message);
    }
}

static void receive_message(int pipe_miso)
{
    char buffer[1024];
    uint16_t len = 0;
    while (em_context.curr_state != EM_STATE_HALT)
    {
        char c;
        ssize_t bytes_read = read(pipe_miso, &c, 1);

        // If no bytes read, continue
        if (bytes_read == 0)
        {
            continue;
        }

        // If not a newline, add character to buffer
        if (c != '\n')
        {
            buffer[len++] = c;
            continue;
        }

        // If newline, and if buffer is not empty, handle message
        if (c == '\n' && len > 0)
        {
            buffer[len] = '\0';
            handle_message(buffer);
            len = 0;
        }
    }
}

int main()
{
    print("Program started");

    int pipe_miso, pipe_simo;
    pid_t pid;
    pthread_t threads[3];

    init_signal();
    init_ports();
    init_cpu_governor();
    init_em();
    init_state();
    init_ui_server(&pipe_miso, &pipe_simo, &pid);
    init_threads(threads);

    // Receive message from UI server, until program is halted
    receive_message(pipe_miso);

    // Join threads
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    print("All threads joined");

    // Kill UI server
    kill(pid, SIGKILL);

    handle_exit();

    return 0;
}