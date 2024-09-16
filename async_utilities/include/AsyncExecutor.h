//
// Created by maxim on 15.09.2024.
//
#pragma once

#include <iostream>
#include <future>
#include "ThreadPool.h"
#include "CallbackDispatcher.h"


template<typename T>
class AsyncExecutor {
public:
    using AsyncOperation = std::function<T()>;
    using Callback = std::function<void(T)>;
    using ExceptionCallback = std::function<void(std::string)>;


    AsyncExecutor(ThreadPool& threadPool, CallbackDispatcher& dispatcher)
        : m_threadPool(threadPool), m_dispatcher(dispatcher) {}


    void start(AsyncOperation operation, Callback callback) {
        std::thread::id current_thread_id = std::this_thread::get_id();
        m_threadPool.enqueue([this, op = std::move(operation), cb = std::move(callback), current_thread_id]() mutable {
            try {
                T result = op();
                m_dispatcher.post([cb = std::move(cb), result]() mutable {
                    try {
                        cb(result);
                    } catch (const std::exception& e) {
                        // Handle callback exception
                        std::cerr << "Callback exception: " << e.what() << std::endl;
                    }
                }, current_thread_id);
            } catch (const std::exception& e) {
                // Handle operation exception
                std::cerr << "Operation exception: " << e.what() << std::endl;
            }
        });
    }

    std::future<T> start(AsyncOperation operation) {
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
    CallbackDispatcher& m_dispatcher;
};


class VoidAsyncExecutor {
public:
    using VoidAsyncOp = std::function<void()>;
    using VoidCallback = std::function<void()>;

    VoidAsyncExecutor(ThreadPool& threadPool, CallbackDispatcher& dispatcher)
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
    CallbackDispatcher& m_dispatcher;
};
