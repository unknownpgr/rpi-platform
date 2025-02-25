cmake_minimum_required(VERSION 3.10)
project(RPi, C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(src/core)
add_executable(app src/main.c src/core/dev.c)

# Add libraries
find_library(LIBGPIOD gpiod)
if(NOT LIBGPIOD)
    message(FATAL_ERROR "libgpiod not found")
endif()
target_link_libraries(app ${LIBGPIOD})
