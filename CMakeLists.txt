cmake_minimum_required(VERSION 3.20)
project(clevo_ns50mu_fan_controller)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(clevo_ns50mu_fan_controller
        NS50MU-fan-controller.cpp)
