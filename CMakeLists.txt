cmake_minimum_required(VERSION 3.10)
project(ps_shell_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pthread")

include_directories(include)

file(GLOB SOURCES "src/*.c")

add_executable(ps_shell_sim ${SOURCES})
target_link_libraries(ps_shell_sim pthread)

add_executable(test_pssim tests/test_pssim.c src/pssim_scheduler.c src/pssim_threads.c src/pssim_memory.c src/pssim_parser.c)
target_link_libraries(test_pssim pthread)
