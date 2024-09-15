//
// Created by maxim on 15.09.2024.
//
#pragma once

#include <iostream>

#include "ThreadPool.h"
#include "Dispatcher.h"


template<typename T>
class AsyncOperation {
public:
    using AsyncOp = std::function<T()>;
    using Callback = std::function<void(T)>;

    AsyncOperation(ThreadPool& threadPool, MainThreadCallbackDispatcher& dispatcher)
        : m_threadPool(threadPool), m_dispatcher(dispatcher) {}

    void start(AsyncOp operation, Callback callback) {
        m_threadPool.enqueue([this, op = std::move(operation), cb = std::move(callback)]() mutable {
            try {
                T result = op();
                m_dispatcher.post([cb = std::move(cb), result]() mutable {
                    try {
                        cb(result);
                    } catch (const std::exception& e) {
                        // Handle callback exception
                        std::cerr << "Callback exception: " << e.what() << std::endl;
                    }
                });
            } catch (const std::exception& e) {
                // Handle operation exception
                std::cerr << "Operation exception: " << e.what() << std::endl;
            }
        });
    }

    std::future<T> start(AsyncOp operation) {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();

        m_threadPool.enqueue([op = std::move(operation), promise]() mutable {
            try {
                T result = op();
                promise->set_value(result);
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
    }

private:
    ThreadPool& m_threadPool;
    MainThreadCallbackDispatcher& m_dispatcher;
};


class VoidAsyncOperation {
public:
    using VoidAsyncOp = std::function<void()>;
    using VoidCallback = std::function<void()>;

    VoidAsyncOperation(ThreadPool& threadPool, MainThreadCallbackDispatcher& dispatcher)
        : m_threadPool(threadPool), m_dispatcher(dispatcher) {}

    void start(VoidAsyncOp operation, VoidCallback callback) const {
        m_threadPool.enqueue([this, op = std::move(operation), cb = std::move(callback)]() mutable {
            try {
                op();
                m_dispatcher.post([cb = std::move(cb)]() mutable {
                    try {
                        cb();
                    } catch (const std::exception& e) {
                        // Handle callback exception
                        std::cerr << "Callback exception: " << e.what() << std::endl;
                    }
                });
            } catch (const std::exception& e) {
                // Handle operation exception
                std::cerr << "Operation exception: " << e.what() << std::endl;
            }
        });
    }

    std::future<void> start(VoidAsyncOp operation)
    {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        m_threadPool.enqueue([op = std::move(operation), promise]() mutable {
            try {
                op();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
    }

private:
    ThreadPool& m_threadPool;
    MainThreadCallbackDispatcher& m_dispatcher;
};
