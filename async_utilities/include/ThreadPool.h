//
// Created by maxim on 15.09.2024.
//
#pragma once
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include "LockFreeQueue.h"

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency())
        : m_threads(threadCount), m_running(true) {
        for (auto& thread : m_threads) {
            thread = std::thread([this]() {
                Task task;
                while (m_running.load(std::memory_order_acquire) || m_queue.dequeue(task)) {
                    if (m_queue.dequeue(task)) {
                        task();
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    template<typename F>
    void enqueue(F&& task) {
        m_queue.enqueue(std::forward<F>(task));
    }

    void shutdown() {
        m_running.store(false, std::memory_order_release);
        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    LockFreeQueue<Task> m_queue;
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_running;
};