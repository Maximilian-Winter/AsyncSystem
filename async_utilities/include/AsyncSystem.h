#pragma once

#include "AsyncExecutor.h"
#include "ThreadPool.h"
#include "CallbackDispatcher.h"
#include <chrono>
#include <functional>
#include <unordered_map>

class AsyncSystem {
public:
    using TaskId = size_t;
    using MainLoopTask = std::function<void()>;
    using AsyncTask = std::function<void()>;
    using CompletionHandler = std::function<void()>;

    AsyncSystem(size_t threadCount = std::thread::hardware_concurrency())
        : m_threadPool(threadCount),
          m_dispatcher(),
          m_asyncExecutor(m_threadPool, m_dispatcher),
          m_running(false),
          m_nextTaskId(0) {}

    void start() {
        m_running = true;
        while (m_running) {
            auto start = std::chrono::steady_clock::now();

            // Run main loop tasks
            for (const auto& task : m_mainLoopTasks) {
                task();
            }

            // Process callbacks
            m_dispatcher.execute_pending();

            // Schedule periodic tasks
            checkAndSchedulePeriodicTasks();

            // Control loop timing
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            if (duration < std::chrono::milliseconds(16)) { // Target 60 FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(16) - duration);
            }
        }
    }

    void stop() {
        m_running = false;
    }

    void addMainLoopTask(MainLoopTask task) {
        m_mainLoopTasks.push_back(std::move(task));
    }

    TaskId scheduleAsyncTask(AsyncTask task, CompletionHandler handler) {
        TaskId id = getNextTaskId();
        m_asyncExecutor.start<void>([task = std::move(task)]() { task(); }, 
            [this, id, handler = std::move(handler)]() {
                m_dispatcher.post([this, id, handler = std::move(handler)]() {
                    handler();
                    m_scheduledTasks.erase(id);
                });
            });
        return id;
    }

    TaskId schedulePeriodicTask(AsyncTask task, CompletionHandler handler, std::chrono::milliseconds interval) {
        TaskId id = getNextTaskId();
        m_scheduledTasks[id] = {std::move(task), std::move(handler), interval, std::chrono::steady_clock::now()};
        return id;
    }

    void cancelTask(TaskId id) {
        m_scheduledTasks.erase(id);
    }

private:
    struct ScheduledTask {
        AsyncTask task;
        CompletionHandler handler;
        std::chrono::milliseconds interval;
        std::chrono::steady_clock::time_point lastRun;
    };

    TaskId getNextTaskId() {
        return m_nextTaskId++;
    }

    void checkAndSchedulePeriodicTasks() {
        auto now = std::chrono::steady_clock::now();
        for (auto& [id, task] : m_scheduledTasks) {
            if (now - task.lastRun >= task.interval) {
                scheduleAsyncTask(task.task, task.handler);
                task.lastRun = now;
            }
        }
    }

    ThreadPool m_threadPool;
    CallbackDispatcher m_dispatcher;
    AsyncExecutor<void> m_asyncExecutor;
    std::vector<MainLoopTask> m_mainLoopTasks;
    std::unordered_map<TaskId, ScheduledTask> m_scheduledTasks;
    std::atomic<bool> m_running;
    std::atomic<TaskId> m_nextTaskId;
};