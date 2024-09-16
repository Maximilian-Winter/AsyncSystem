//
// Created by maxim on 15.09.2024.
//
#pragma once

#include <iostream>
#include <future>
#include <optional>
#include <sstream>

#include "ThreadPool.h"
#include "CallbackDispatcher.h"
#include "CancellableOperation.h"

template<typename T>
class AsyncExecutor
{
public:
    using AsyncOperation = std::function<T()>;
    using Callback = std::function<void(T)>;
    using ExceptionCallback = std::function<void(std::string)>;


    AsyncExecutor(ThreadPool &threadPool, CallbackDispatcher &dispatcher)
        : m_threadPool(threadPool), m_dispatcher(dispatcher)
    {
    }


    std::shared_ptr<CancellableOperation<T> > start(AsyncOperation operation, std::optional<Callback> callback =
                                                            std::nullopt,
                                                    const std::optional<ExceptionCallback> &exception_callback =
                                                            std::nullopt)
    {
        auto cancellableOp = std::make_shared<CancellableOperation<T> >(std::move(operation), std::move(callback));
        std::thread::id current_thread_id = std::this_thread::get_id();

        m_threadPool.enqueue(
            [this, cancellableOp, current_thread_id, exception_callback]() mutable
            {
                if (cancellableOp->isCancelled())
                {
                    return;
                }

                try
                {
                    T result = cancellableOp->execute();
                    if (cancellableOp->isCancelled() || !cancellableOp->hasCallback())
                    {
                        return;
                    }
                    m_dispatcher.post([cancellableOp, result, exception_callback]() mutable
                    {
                        if (cancellableOp->isCancelled())
                        {
                            return;
                        }
                        try
                        {
                            cancellableOp->callback(result);
                        } catch (const std::exception &e)
                        {
                            std::ostringstream oss;
                            oss << "Callback exception: " << e.what() << std::endl;
                            const std::string error_message = oss.str();
                            if (exception_callback)
                            {
                                (*exception_callback)(error_message);
                            } else
                            {
                                std::cerr << error_message;
                            }
                            cancellableOp->setPromiseException(std::current_exception());
                        }
                    }, current_thread_id);
                } catch (const std::exception &e)
                {
                    std::ostringstream oss;
                    oss << "Operation exception: " << e.what() << std::endl;
                    const std::string error_message = oss.str();
                    if (exception_callback)
                    {
                        (*exception_callback)(error_message);
                    } else
                    {
                        std::cerr << error_message;
                    }
                    cancellableOp->setPromiseException(std::current_exception());
                }
            });

        return cancellableOp;
    }

    void shutdown() const
    {
        m_threadPool.shutdown();
        m_dispatcher.stop();
    }

private:
    ThreadPool &m_threadPool;
    CallbackDispatcher &m_dispatcher;
};


class VoidAsyncExecutor
{
public:
    using AsyncOperation = std::function<void()>;
    using Callback = std::function<void()>;
    using ExceptionCallback = std::function<void(std::string)>;


    VoidAsyncExecutor(ThreadPool &threadPool, CallbackDispatcher &dispatcher)
        : m_threadPool(threadPool), m_dispatcher(dispatcher), m_isRunning(true)
    {
    }


    std::shared_ptr<VoidCancellableOperation> start(const AsyncOperation& operation, std::optional<Callback> callback =
                                                            std::nullopt,
                                                    const std::optional<ExceptionCallback> &exception_callback =
                                                            std::nullopt)
    {
        auto cancellableOp = std::make_shared<VoidCancellableOperation>(operation, std::move(callback));
        std::thread::id current_thread_id = std::this_thread::get_id();

        m_threadPool.enqueue(
            [this, cancellableOp, current_thread_id, exception_callback]() mutable
            {
                if (cancellableOp->isCancelled())
                {
                    return;
                }

                try
                {
                    cancellableOp->execute();
                    if (cancellableOp->isCancelled() || !cancellableOp->hasCallback())
                    {
                        return;
                    }
                    m_dispatcher.post([cancellableOp, exception_callback]() mutable
                    {
                        if (cancellableOp->isCancelled())
                        {
                            return;
                        }
                        try
                        {
                            cancellableOp->callback();
                        } catch (const std::exception &e)
                        {
                            std::ostringstream oss;
                            oss << "Callback exception: " << e.what() << std::endl;
                            const std::string error_message = oss.str();
                            if (exception_callback)
                            {
                                (*exception_callback)(error_message);
                            } else
                            {
                                std::cerr << error_message;
                            }
                            cancellableOp->setPromiseException(std::current_exception());
                        }
                    }, current_thread_id);
                } catch (const std::exception &e)
                {
                    std::ostringstream oss;
                    oss << "Operation exception: " << e.what() << std::endl;
                    const std::string error_message = oss.str();
                    if (exception_callback)
                    {
                        (*exception_callback)(error_message);
                    } else
                    {
                        std::cerr << error_message;
                    }
                    cancellableOp->setPromiseException(std::current_exception());
                }
            });

        return cancellableOp;
    }

    void shutdown()
    {
        m_threadPool.shutdown();
        m_dispatcher.stop();
        m_isRunning = false;
    }

    [[nodiscard]] bool isRunning() const
    {
        return m_isRunning;
    }
private:
    bool m_isRunning;
    ThreadPool &m_threadPool;
    CallbackDispatcher &m_dispatcher;
};
