cmake_minimum_required(VERSION 3.10)
project(RPi, C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(src/core)
add_executable(
    app
    src/main.c
    src/core/dev.c
    src/core/log.c
    src/infra/motor.c
    src/infra/encoder.c
    src/infra/timer.c
)
target_link_libraries(app m)