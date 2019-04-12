cmake_minimum_required(VERSION 3.13)
set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required
project(processSch C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(processSch
        src/os-sim.c
        src/os-sim.h
        src/process.c
        src/process.h
        src/student.c
        src/student.h)
