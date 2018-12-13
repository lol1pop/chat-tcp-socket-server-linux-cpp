cmake_minimum_required(VERSION 3.13)
project(server)

set(CMAKE_CXX_STANDARD 14)

add_executable(server main.cpp)

add_compile_options( -pthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)