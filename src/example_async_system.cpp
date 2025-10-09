//
// Created by maxim on 25.09.2024.
//
#include "AsyncSystem.h"
#include <iostream>
#include <chrono>
#include <atomic>

int main() {
    AsyncSystem system;

    // Set main loop
    system.setMainLoop([]() {
        std::cout << "Main loop tick" << std::endl;
    }, std::chrono::seconds(1));

    // Schedule a task
    system.scheduleTask([]() {
        std::cout << "Scheduled task running" << std::endl;
    }, std::chrono::milliseconds(500));

    // Execute an async task
    system.executeAsync<int>([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 42;
    }, [](int result) {
        std::cout << "Async task completed with result: " << result << std::endl;
    });

    // Run the system
    system.run();

    return 0;
}