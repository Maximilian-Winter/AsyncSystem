#include <iostream>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <future>
#include <atomic>
#include "Dispatcher.h"


class TaskQueue {
public:
    using Task = std::function<void()>;

    void push(Task task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_condition.notify_one();
    }

    bool pop(Task& task) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_condition.wait(lock, [this]() {
            return !m_tasks.empty() || m_stopped;
        });

        if (m_stopped && m_tasks.empty())
            return false;

        task = std::move(m_tasks.front());
        m_tasks.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
        }
        m_condition.notify_all();
    }

private:
    std::queue<Task> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopped = false;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency()) : m_queue(), m_threads(threadCount) {
        for (auto& thread : m_threads) {
            thread = std::thread([this]() {
                TaskQueue::Task task;
                while (m_queue.pop(task)) {
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        m_queue.stop();
        for (auto& thread : m_threads) {
            if (thread.joinable())
                thread.join();
        }
    }

    template<typename F>
    void enqueue(F&& task) {
        m_queue.push(std::forward<F>(task));
    }

private:
    TaskQueue m_queue;
    std::vector<std::thread> m_threads;
};

template<typename T>
class AsyncOperation {
public:
    using AsyncOp = const std::function<T()>;
    using Callback = const std::function<void(T)>;

    AsyncOperation(ThreadPool& threadPool, Dispatcher& dispatcher)
       : m_threadPool(threadPool), m_dispatcher(dispatcher) {}

    void start(AsyncOp& operation, Callback& callback) {
        m_threadPool.enqueue([this, operation, callback]() {
            T result = operation();
            m_dispatcher.post([callback, result]() {
                callback(result);
            });
        });
    }

    std::future<T> start(AsyncOp operation) {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();

        m_threadPool.enqueue([operation, promise]() {
            try {
                T result = operation();
                promise->set_value(result);
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
    }
private:
    ThreadPool& m_threadPool;
    Dispatcher& m_dispatcher;
};


class VoidAsyncOperation {
public:
    using VoidAsyncOp = std::function<void()>;
    using VoidCallback = std::function<void()>;

    explicit VoidAsyncOperation(ThreadPool& threadPool, Dispatcher& dispatcher)
       : m_threadPool(threadPool), m_dispatcher(dispatcher) {}

    void start(const VoidAsyncOp& operation, const VoidCallback& callback) const
    {
        // Enqueue the asynchronous task
        m_threadPool.enqueue([this, operation, callback]() {
            operation();
            m_dispatcher.post([callback]() {
                callback();
            });
        });
    }
private:
    ThreadPool& m_threadPool;
    Dispatcher& m_dispatcher;
};


int main() {
    std::cout << "Starting advanced asynchronous system..." << std::endl;

    // Create a thread pool
    ThreadPool threadPool;

    // Create a dispatcher
    Dispatcher dispatcher;

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
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return x * x;
        }, completionHandler);
    }

    std::cout << "Asynchronous operations initiated. Main thread is free to continue..." << std::endl;

    // Main thread event loop to process dispatcher tasks
    // Break condition (for demonstration purposes)
    static int count = 0;
    while (true) {
        // Process any pending callbacks
        dispatcher.execute_pending();

        // Do other work in the main thread
        std::cout << "Main thread working..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));


        if (++count >= 10) {
            dispatcher.stop(); // Stop the dispatcher
            break;
        }
    }

    // Ensure all tasks have been processed before exiting
    while (!dispatcher.is_stopped()) {
        dispatcher.execute_pending();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Destructor of ThreadPool will wait for all tasks to complete
    std::cout << "Main thread completed." << std::endl;
    return 0;
}

