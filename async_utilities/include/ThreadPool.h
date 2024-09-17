#pragma once
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "LockFreeQueue.h"

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency())
        : m_threads(threadCount), m_running(true), m_idleThreads(threadCount) {
        for (auto& thread : m_threads) {
            thread = std::thread([this]() {
                while (m_running) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_condition.wait(lock, [this] { return !m_running || !m_queue.is_empty(); });

                        if (!m_running && m_queue.is_empty()) {
                            return;
                        }

                        auto task_opt = m_queue.dequeue();
                        if (!task_opt) {
                            continue;
                        }
                        task = std::move(*task_opt);
                    }

                    m_idleThreads--;
                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
                    }
                    m_idleThreads++;
                }
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    void enqueue(Task task) {
        if (!task) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.enqueue(std::move(task));
        }
        m_condition.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_running = false;
        }
        m_condition.notify_all();
        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    size_t get_idle_thread_count() const {
        return m_idleThreads.load();
    }

private:
    LockFreeQueue<Task> m_queue;
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_running;
    std::atomic<size_t> m_idleThreads;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};