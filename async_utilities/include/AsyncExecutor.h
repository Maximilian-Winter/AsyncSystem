#pragma once

#include <iostream>
#include <future>
#include <optional>
#include <sstream>
#include <memory>
#include <atomic>

#include "ThreadPool.h"
#include "CallbackDispatcher.h"

template<typename T>
class CancellableOperation {
public:
    using Operation = std::function<T()>;
    using Callback = std::function<void(T)>;

    explicit CancellableOperation(Operation op, std::optional<Callback> cb)
        : m_operation(std::move(op)), m_callback(std::move(cb)), m_isCancelled(false), m_isFinished(false), m_ResultPromise(std::make_shared<std::promise<T>>()) {}

    T execute() {
        if (!m_isCancelled.load(std::memory_order_acquire)) {
            T result = m_operation();
            setPromiseValue(result);
            return result;
        }
        throw std::runtime_error("Operation cancelled");
    }

    void callback(const T& value) {
        if (!m_isCancelled.load(std::memory_order_acquire) && !m_isFinished.exchange(true)) {
            if(m_callback) {
                (*m_callback)(value);
            }
        }
    }

    void cancel() {
        m_isCancelled.store(true, std::memory_order_release);
    }

    bool isCancelled() const {
        return m_isCancelled.load(std::memory_order_acquire);
    }

    bool isFinished() const {
        return m_isFinished.load(std::memory_order_acquire);
    }

    bool hasCallback() const {
        return static_cast<bool>(m_callback);
    }

    void setPromiseException(std::exception_ptr e) {
        if (!isFinished()) {
            m_ResultPromise->set_exception(std::move(e));
        }
    }

    std::future<T> getFuture() {
        return m_ResultPromise->get_future();
    }

private:
    void setPromiseValue(const T& value) {
        if (!isFinished()) {
            m_ResultPromise->set_value(value);
        }
    }

    Operation m_operation;
    std::optional<Callback> m_callback;
    std::atomic<bool> m_isCancelled;
    std::atomic<bool> m_isFinished;
    std::shared_ptr<std::promise<T>> m_ResultPromise;
};

template<typename T>
class AsyncExecutor {
public:
    using AsyncOperation = std::function<T()>;
    using Callback = std::function<void(T)>;
    using ExceptionCallback = std::function<void(std::string)>;

    AsyncExecutor(ThreadPool& threadPool, CallbackDispatcher& dispatcher)
        : m_threadPool(threadPool), m_dispatcher(dispatcher) {}

    std::shared_ptr<CancellableOperation<T>> start(AsyncOperation operation, std::optional<Callback> callback = std::nullopt,
                                                   const std::optional<ExceptionCallback>& exception_callback = std::nullopt) {
        auto cancellableOp = std::make_shared<CancellableOperation<T>>(std::move(operation), std::move(callback));
        std::thread::id current_thread_id = std::this_thread::get_id();

        m_threadPool.enqueue([this, cancellableOp, current_thread_id, exception_callback]() mutable {
            if (cancellableOp->isCancelled()) {
                return;
            }

            try {
                T result = cancellableOp->execute();
                if (cancellableOp->isCancelled() || !cancellableOp->hasCallback()) {
                    return;
                }
                m_dispatcher.post([cancellableOp, result, exception_callback]() mutable {
                    bool cancelled = cancellableOp->isCancelled();
                    bool finished = cancellableOp->isFinished();

                    if (cancelled || finished) {
                        return;
                    }
                    try {
                        cancellableOp->callback(result);
                    } catch (const std::exception& e) {
                        std::ostringstream oss;
                        oss << "Callback exception: " << e.what() << std::endl;
                        const std::string error_message = oss.str();
                        if (exception_callback) {
                            (*exception_callback)(error_message);
                        } else {
                            std::cerr << error_message;
                        }
                    }
                }, current_thread_id);
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Operation exception: " << e.what() << std::endl;
                const std::string error_message = oss.str();
                if (exception_callback) {
                    (*exception_callback)(error_message);
                } else {
                    std::cerr << error_message;
                }
                cancellableOp->setPromiseException(std::current_exception());
            }
        });

        return cancellableOp;
    }

    void shutdown() const {
        m_threadPool.shutdown();
        m_dispatcher.stop();
    }

private:
    ThreadPool& m_threadPool;
    CallbackDispatcher& m_dispatcher;
};