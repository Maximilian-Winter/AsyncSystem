#include "AsyncOperation.h"
#include <iostream>

int main() {
    std::cout << "Starting advanced asynchronous system..." << std::endl;

    // Create a thread pool
    ThreadPool threadPool;

    // Create a dispatcher
    MainThreadCallbackDispatcher dispatcher;

    // Create an AsyncOperation instance
    AsyncOperation<int> asyncOp(threadPool, dispatcher);

    // Define the completion handler
    auto completionHandler = [](int result) {
        std::cout << "Asynchronous operation completed with result: " << result << std::endl;
    };

    int x = 8;

    // Start multiple asynchronous operations
    for (int i = 0; i < 5; ++i) {
        asyncOp.start([x]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return x * x;
        }, completionHandler);
    }

    std::cout << "Asynchronous operations initiated. Main thread is free to continue..." << std::endl;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(asyncOp.start([x]() {
            std::this_thread::sleep_for(std::chrono::seconds(4));
            return x * x;
        }));
    }

    // Later, collect results
    for (auto& future : futures) {
        int result = future.get(); // Will wait until the result is available
        std::cout << "Future result: " << result << std::endl;
    }


    while (!dispatcher.is_stopped()) {
        dispatcher.execute_pending();
        dispatcher.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Destructor of ThreadPool will wait for all tasks to complete
    std::cout << "Main thread completed." << std::endl;
    return 0;
}

