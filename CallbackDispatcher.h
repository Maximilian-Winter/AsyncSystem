//
// Created by maxim on 15.09.2024.
//
#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

struct CallbackInfo {
    std::function<void()> task;
    std::thread::id thread_id;

    CallbackInfo(std::function<void()> t, std::thread::id id)
        : task(std::move(t)), thread_id(id) {}
};

class CallbackDispatcher {
public:
    CallbackDispatcher() : m_stopped(false) {}

    void post(std::function<void()> task, std::thread::id thread_id = std::thread::id()) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.emplace(std::move(task), thread_id);
        }
        m_condition.notify_one();
    }

    void execute_pending(size_t max_tasks = std::numeric_limits<size_t>::max()) {
        std::thread::id current_thread_id = std::this_thread::get_id();
        std::queue<CallbackInfo> tasks_to_execute;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_tasks.empty() && tasks_to_execute.size() < max_tasks) {
                if (m_tasks.front().thread_id == std::thread::id() ||
                    m_tasks.front().thread_id == current_thread_id) {
                    tasks_to_execute.push(std::move(m_tasks.front()));
                    m_tasks.pop();
                } else {
                    // If the task is for a different thread, move it to the back of the queue
                    m_tasks.push(std::move(m_tasks.front()));
                    m_tasks.pop();
                }
            }
        }

        while (!tasks_to_execute.empty()) {
            tasks_to_execute.front().task();
            tasks_to_execute.pop();
        }
    }

    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_tasks.empty()) {
            m_condition.wait(lock, [this]() { return !m_tasks.empty() || m_stopped; });
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
        }
        m_condition.notify_all();
    }

    bool is_stopped() const {
        return m_stopped;
    }

private:
    std::queue<CallbackInfo> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopped;
};