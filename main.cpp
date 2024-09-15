#include "AsyncExecutor.h"
#include <iostream>

int main() {
    std::cout << "Starting advanced asynchronous system..." << std::endl;

    // Create a thread pool
    ThreadPool threadPool;

    // Create a dispatcher
    CallbackDispatcher dispatcher;

    // Create an AsyncExecutor instance
    AsyncExecutor<int> asyncExecutor(threadPool, dispatcher);

    // Define the completion handler
    auto completionHandler = [](int result) {
        std::cout << "Asynchronous operation completed with result: " << result << std::endl;
    };

    int x = 8;

    // Start multiple asynchronous operations
    for (int i = 0; i < 5; ++i) {
        asyncExecutor.start([x]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return x * x;
        }, completionHandler);
    }

    std::cout << "Asynchronous operations initiated. Main thread is free to continue..." << std::endl;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(asyncExecutor.start([x]() {
            std::this_thread::sleep_for(std::chrono::seconds(4));
            return x * x;
        }));
    }

    // Later, collect results
    for (auto& future : futures) {
        int result = future.get(); // Will wait until the result is available
        std::cout << "Future result: " << result << std::endl;
    }
    dispatcher.execute_pending();


    while (!dispatcher.is_stopped()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    dispatcher.stop();
    // Destructor of ThreadPool will wait for all tasks to complete
    std::cout << "Main thread completed." << std::endl;
    return 0;
}

