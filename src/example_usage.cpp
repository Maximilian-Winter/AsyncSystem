#include "AsyncExecutor.h"
#include <iostream>
#include <chrono>
#include <atomic>

int main() {
    std::cout << "Starting advanced asynchronous system..." << std::endl;

    // Create a dispatcher
    CallbackDispatcher dispatcher;

    // Create a thread pool
    ThreadPool threadPool(1000);

    // Create an AsyncExecutor instance
    AsyncExecutor<int> asyncExecutor(threadPool, dispatcher);

    // Define the completion handler
    auto completionHandler = [](int result) {
        std::cout << "Asynchronous operation completed with result: " << result << std::endl;
    };

    std::atomic<int> completed_tasks(0);
    const int total_tasks = 1000;

    std::vector<std::shared_ptr<CancellableOperation<int>>> operations;
    operations.reserve(total_tasks);

    // Start multiple asynchronous operations
    for (int i = 0; i < total_tasks; ++i) {
        operations.push_back(asyncExecutor.start([i, &completed_tasks]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ++completed_tasks;
            return i;
        }, completionHandler));
    }

    std::cout << "Asynchronous operations initiated. Main thread is processing callbacks..." << std::endl;

    // Process callbacks on the main thread
    /*while (completed_tasks < total_tasks || dispatcher.has_pending_tasks()) {
        dispatcher.execute_pending();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }*/

    std::cout << "All callbacks processed. Now collecting future results..." << std::endl;

    // Now, collect results from futures
    for (auto& op : operations) {
        int result = op->getFuture().get(); // Will return immediately as all tasks are completed
        std::cout << "Future result: " << result << std::endl;
    }

    // Stop the dispatcher and thread pool
    dispatcher.stop();
    threadPool.shutdown();

    std::cout << "Main thread completed." << std::endl;
    return 0;
}