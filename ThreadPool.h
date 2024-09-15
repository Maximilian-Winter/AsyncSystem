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
#include <atomic>
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
