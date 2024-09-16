//
// Created by maxim on 16.09.2024.
//

#pragma once
#include <atomic>
#include <functional>
#include <utility>

template<typename T>
class CancellableOperation {
public:
    using Operation = std::function<T()>;
    using Callback = std::function<void(T)>;

    explicit CancellableOperation(Operation op, std::optional<Callback> cb)
        : m_operation(std::move(op)), m_callback(cb), m_isCancelled(false), m_isFinished(false), m_ResultPromise(std::make_shared<std::promise<T>>()) {}

    T execute() {
        if (!m_isCancelled.load(std::memory_order_acquire)) {
            T result = m_operation();
            setPromiseValue(result);
            return result;
        }
        return {};
    }
    void callback(T value)
    {
        if (!m_isCancelled.load(std::memory_order_acquire)) {
            if(hasCallback())
                (*m_callback)(value);
        }
        finish();
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
        if(m_callback)
            return true;
        return false;
    }

    void setPromiseException(std::exception_ptr e) {
        m_ResultPromise->set_exception(e);
    }

    std::future<T> getFuture()
    {
        return m_ResultPromise->get_future();
    }

private:
    void finish() {
        m_isFinished.store(true, std::memory_order_release);
    }
    void setPromiseValue(T value)
    {
        m_ResultPromise->set_value(value);
    }
    std::shared_ptr<std::promise<T>> m_ResultPromise;
    Operation m_operation;
    std::optional<Callback> m_callback;
    std::atomic<bool> m_isCancelled;
    std::atomic<bool> m_isFinished;
};

class VoidCancellableOperation {
public:
    using Operation = std::function<void()>;
    using Callback = std::function<void()>;

    explicit VoidCancellableOperation(Operation op, std::optional<Callback> cb)
        : m_operation(std::move(op)), m_callback(std::move(cb)), m_isCancelled(false), m_isFinished(false), m_ResultPromise(std::make_shared<std::promise<void>>()) {}

    void execute() const
    {
        if (!m_isCancelled.load(std::memory_order_acquire)) {
            m_operation();
            setPromiseValue();
        }

    }
    void callback()
    {
        if (!m_isCancelled.load(std::memory_order_acquire)) {
            if(hasCallback())
                (*m_callback)();
        }
        finish();
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
        if(m_callback)
            return true;
        return false;
    }
    std::future<void> getFuture() const
    {
        return m_ResultPromise->get_future();
    }
    void setPromiseException(const std::exception_ptr& e) const
    {
        m_ResultPromise->set_exception(e);
    }
private:
    void finish() {
        m_isFinished.store(true, std::memory_order_release);
    }
    void setPromiseValue() const
    {
        m_ResultPromise->set_value();
    }
    std::shared_ptr<std::promise<void>> m_ResultPromise;
    Operation m_operation;
    std::optional<Callback> m_callback;
    std::atomic<bool> m_isCancelled;
    std::atomic<bool> m_isFinished;
};
