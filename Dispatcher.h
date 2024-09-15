//
// Created by maxim on 15.09.2024.
//
#pragma once
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

class Dispatcher {
public:
    using Task = std::function<void()>;

    // Post a task to the dispatcher
    void post(Task task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_condition.notify_one();
    }

    // Execute all pending tasks
    void execute_pending() {
        std::queue<Task> tasks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::swap(tasks, m_tasks);
        }

        while (!tasks.empty()) {
            tasks.front()();
            tasks.pop();
        }
    }

    // Wait for tasks to be available
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_tasks.empty()) {
            m_condition.wait(lock, [this]() { return !m_tasks.empty() || m_stopped; });
        }
    }

    // Stop the dispatcher
    void stop() {
        {
            std::lock_guard lock(m_mutex);
            m_stopped = true;
        }
        m_condition.notify_all();
    }

    [[nodiscard]] bool is_stopped() const {
        return m_stopped;
    }

private:
    std::queue<Task> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopped = false;
};
