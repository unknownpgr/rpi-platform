cmake_minimum_required(VERSION 3.10)
project(RPi, C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Turn off GNU extensions
set(CMAKE_C_EXTENSIONS OFF)

# Enable POSIX
add_compile_definitions(_POSIX_C_SOURCE=200809L)

include_directories(src-main/core/interfaces)
include_directories(src-main/core)

add_executable(
    app
    
    src-main/entry.c
    src-main/main.c
    src-main/core/em.c  
    src-main/core/state.c
    
    # src-main/core/services/knob.c
    src-main/core/services/music.c
    src-main/core/services/radio.c
    src-main/core/services/sensor.c
    src-main/core/services/drive.c
    src-main/core/services/vsense.c
    src-main/core/services/imu.c
    src-main/core/services/clock.c
    src-main/core/services/encoder.c
    
    src-main/infra/dev.c
    src-main/infra/log.c
    src-main/infra/motor.c
    src-main/infra/timer.c
)
target_link_libraries(app m)
find_package(Threads REQUIRED)
target_link_libraries(app ${CMAKE_THREAD_LIBS_INIT})