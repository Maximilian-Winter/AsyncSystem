cmake_minimum_required(VERSION 3.29)
project(AsyncSystem)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        ${CMAKE_SOURCE_DIR}/async_utilities/include
)

# Main executable

add_executable(AsyncSystemExample src/example_async_system.cpp)

