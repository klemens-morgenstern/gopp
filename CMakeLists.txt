cmake_minimum_required(VERSION 3.23)
project(gopp)

find_package(Boost REQUIRED context)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(test)