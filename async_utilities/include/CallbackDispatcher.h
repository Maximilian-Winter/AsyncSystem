#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "LockFreeQueue.h"

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
            m_tasks.enqueue(CallbackInfo(std::move(task), thread_id));
        }
        m_condition.notify_one();
    }

    bool execute_pending(size_t max_tasks = std::numeric_limits<size_t>::max()) {
        std::thread::id current_thread_id = std::this_thread::get_id();
        std::queue<CallbackInfo> tasks_to_execute;
        bool tasks_executed = false;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_tasks.is_empty() && tasks_to_execute.size() < max_tasks) {
                auto task = m_tasks.dequeue();
                if (task) {
                    if (task->thread_id == std::thread::id() ||
                        task->thread_id == current_thread_id)
                    {
                        tasks_to_execute.push(std::move(*task));
                    } else {
                        m_tasks.enqueue(*task);
                    }
                }
            }
        }

        while (!tasks_to_execute.empty()) {
            tasks_to_execute.front().task();
            tasks_to_execute.pop();
            tasks_executed = true;
        }

        return tasks_executed;
    }

    bool has_pending_tasks() const {
        return !m_tasks.is_empty();
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
    LockFreeQueue<CallbackInfo> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stopped;
};