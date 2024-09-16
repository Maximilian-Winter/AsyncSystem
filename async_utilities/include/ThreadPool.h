//
// Created by maxim on 15.09.2024.
//
#pragma once
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <future>

#include "LockFreeQueue.h"




class ThreadPool {
public:
    using Task = std::function<void()>;
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency()) : m_queue(), m_threads(threadCount) {
        for (auto& thread : m_threads) {
            thread = std::thread([this]() {
                Task task;
                while (m_queue.dequeue(task)) {
                    task();
                }
            });
        }
    }

    ~ThreadPool() {

        for (auto& thread : m_threads) {
            if (thread.joinable())
                thread.join();
        }
    }

    template<typename F>
    void enqueue(F&& task) {
        m_queue.enqueue(std::forward<F>(task));
    }

private:
    LockFreeQueue<Task> m_queue;
    std::vector<std::thread> m_threads;
};
