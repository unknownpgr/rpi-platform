cmake_minimum_required(VERSION 3.10)
project(RPi, C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(src/core/interfaces)
add_executable(
    app
    
    src/entry.c
    src/main.c
    
    src/core/services/log.c
    src/core/services/knob-service.c
    src/core/services/music-service.c
    src/core/services/radio-service.c
    src/core/services/sensor-service.c
    src/core/services/keyboard-service.c
    src/core/services/drive-service.c
    src/core/services/vsense-service.c
    src/core/services/imu-service.c

    src/infra/dev.c
    src/infra/motor.c
    src/infra/encoder.c
    src/infra/timer.c
    src/infra/linalg.c
)
target_link_libraries(app m)
find_package(Threads REQUIRED)
find_package(LAPACK REQUIRED)
target_link_libraries(app ${CMAKE_THREAD_LIBS_INIT} ${LAPACK_LIBRARIES})