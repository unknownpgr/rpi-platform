cmake_minimum_required(VERSION 3.10)
project(RPi, C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Turn off GNU extensions
set(CMAKE_C_EXTENSIONS OFF)

# Enable POSIX
add_compile_definitions(_POSIX_C_SOURCE=200809L)

include_directories(src-main/core/include)

add_executable(
    app
    
    main/entry.c
    main/main.c
    main/core/em.c  
    main/core/state.c
    
    # main/core/services/knob.c
    main/core/src/services/music.c
    main/core/src/services/radio.c
    main/core/src/services/sensor.c
    main/core/src/services/drive.c
    main/core/src/services/vsense.c
    main/core/src/services/imu.c
    main/core/src/services/clock.c
    main/core/src/services/encoder.c

    main/core/src/algorithms/mark.c
    main/core/src/algorithms/pid.c
    
    main/infra/dev.c
    main/infra/log.c
    main/infra/motor.c
    main/infra/timer.c
)
target_link_libraries(app m)
find_package(Threads REQUIRED)
target_link_libraries(app ${CMAKE_THREAD_LIBS_INIT})