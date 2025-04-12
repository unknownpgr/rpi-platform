cmake_minimum_required(VERSION 3.10)
project(RPi, C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(src-main/core/interfaces)
add_executable(
    app
    
    src-main/entry.c
    src-main/main.c
    
    src-main/core/services/log.c
    src-main/core/services/knob-service.c
    src-main/core/services/music-service.c
    src-main/core/services/radio-service.c
    src-main/core/services/sensor-service.c
    src-main/core/services/keyboard-service.c
    src-main/core/services/drive-service.c
    src-main/core/services/vsense-service.c
    src-main/core/services/imu-service.c

    src-main/infra/dev.c
    src-main/infra/motor.c
    src-main/infra/encoder.c
    src-main/infra/timer.c
    src-main/infra/linalg.c
)
target_link_libraries(app m)
find_package(Threads REQUIRED)
find_package(LAPACK REQUIRED)
target_link_libraries(app ${CMAKE_THREAD_LIBS_INIT} ${LAPACK_LIBRARIES})