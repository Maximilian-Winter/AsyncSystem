//
// Created by maxim on 15.09.2024.
//
#pragma once
#include <future>
#include <atomic>
#include <memory>
#include <queue>


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
