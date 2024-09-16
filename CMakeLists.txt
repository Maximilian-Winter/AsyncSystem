cmake_minimum_required(VERSION 3.29)
project(AsyncSystem)

set(CMAKE_CXX_STANDARD 20)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Main executable
add_executable(AsyncSystem main.cpp)

# Test executable
add_executable(AsyncSystemTests
        lock_free_tests.cpp
        # Add other source files needed for testing, e.g.:
        # LockFreeQueue.h
        # LockFreeList.h
        # LockFreeStack.h
)

# Link test executable against Google Test
target_link_libraries(AsyncSystemTests GTest::gtest_main)

# Enable testing
enable_testing()

# Discover tests
include(GoogleTest)
gtest_discover_tests(AsyncSystemTests)
