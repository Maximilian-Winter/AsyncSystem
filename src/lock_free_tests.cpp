//
// Created by maxim on 16.09.2024.
//
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>
#include "LockFreeQueue.h"
#include "LockFreeList.h"
#include "LockFreeStack.h"

const int NUM_THREADS = 4;
const int OPERATIONS_PER_THREAD = 10000;
const int STRESS_TEST_DURATION = 5; // seconds

// Test for LockFreeQueue
TEST(LockFreeQueueTest, MultiThreadedEnqueueDequeue) {
    LockFreeQueue<int> queue;
    std::vector<std::thread> threads;
    std::atomic<int> enqueued_sum(0);
    std::atomic<int> dequeued_sum(0);
    std::atomic<int> enqueue_count(0);
    std::atomic<int> dequeue_count(0);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&queue, &enqueued_sum, &enqueue_count, i]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int value = i * OPERATIONS_PER_THREAD + j;
                queue.enqueue(value);
                enqueued_sum.fetch_add(value);
                enqueue_count.fetch_add(1);
            }
        });
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&queue, &dequeued_sum, &dequeue_count]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int value;
                while (!queue.dequeue(value)) {
                    std::this_thread::yield();
                }
                dequeued_sum.fetch_add(value);
                dequeue_count.fetch_add(1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(enqueue_count.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
    EXPECT_EQ(dequeue_count.load(), NUM_THREADS * OPERATIONS_PER_THREAD);
    EXPECT_EQ(enqueued_sum.load(), dequeued_sum.load());
}


// Test for LockFreeList
TEST(LockFreeListTest, MultiThreadedInsertRemove) {
    LockFreeList<int> list;
    std::vector<std::thread> threads;
    std::atomic<int> sum(0);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&list, &sum, i]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int value = i * OPERATIONS_PER_THREAD + j;
                list.insert_beginning(value);
                sum.fetch_add(value, std::memory_order_relaxed);
            }
        });
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&list, &sum]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, NUM_THREADS * OPERATIONS_PER_THREAD - 1);

            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int value = dis(gen);
                if (list.remove(value)) {
                    sum.fetch_sub(value, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Check remaining elements
    int remaining_sum = 0;
    for (int i = 0; i < NUM_THREADS * OPERATIONS_PER_THREAD; ++i) {
        if (list.find(i).has_value()) {
            remaining_sum += i;
        }
    }

    EXPECT_EQ(sum.load(), remaining_sum);
}

// Test for LockFreeStack
TEST(LockFreeStackTest, MultiThreadedPushPop) {
    LockFreeStack<int> stack;
    std::vector<std::thread> threads;
    //std::atomic<int> sum(0);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&stack, i]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int value = i * OPERATIONS_PER_THREAD + j;
                stack.push(value);
                //sum.fetch_add(value, std::memory_order_relaxed);
            }
        });
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&stack]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                std::optional<int> value = stack.pop();
                while (!value) {
                    std::this_thread::yield();
                    value = stack.pop();
                }
                //sum.fetch_sub(*value, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    //EXPECT_EQ(sum.load(), 0);
    EXPECT_TRUE(stack.is_empty());
}


// Additional test for LockFreeQueue: Ensure FIFO order
TEST(LockFreeQueueTest, FIFOOrder) {
    LockFreeQueue<int> queue;
    const int NUM_ELEMENTS = 1000;

    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        queue.enqueue(i);
    }

    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        int value;
        ASSERT_TRUE(queue.dequeue(value));
        EXPECT_EQ(value, i);
    }
}

// Additional test for LockFreeList: Concurrent insert and find
TEST(LockFreeListTest, ConcurrentInsertFind) {
    LockFreeList<int> list;
    std::vector<std::thread> threads;
    std::atomic<bool> start{false};
    std::atomic<int> found_count{0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&list, &start, i]() {
            while (!start) { std::this_thread::yield(); }
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                list.insert_beginning(i * OPERATIONS_PER_THREAD + j);
            }
        });
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&list, &start, &found_count]() {
            while (!start) { std::this_thread::yield(); }
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                if (list.find(j).has_value()) {
                    found_count.fetch_add(1);
                }
            }
        });
    }

    start = true;
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(found_count.load(), 0);
}

// Additional test for LockFreeStack: Ensure LIFO order
TEST(LockFreeStackTest, LIFOOrder) {
    LockFreeStack<int> stack;
    const int NUM_ELEMENTS = 1000;

    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        stack.push(i);
    }

    for (int i = NUM_ELEMENTS - 1; i >= 0; --i) {
        auto value = stack.pop();
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, i);
    }
}

// Stress test for LockFreeQueue
std::mutex cout_mutex;

#define SAFE_COUT(x) { std::lock_guard<std::mutex> lock(cout_mutex); std::cout << x << std::endl; }

// Updated stress test for LockFreeQueue
TEST(LockFreeQueueStressTest, ContinuousEnqueueDequeue) {
    LockFreeQueue<int> queue;
    std::atomic<bool> stop{false};
    std::atomic<long long> total_operations{0};
    std::atomic<bool> error_occurred{false};

    auto worker = [&](bool is_producer, int thread_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);

        try {
            while (!stop && !error_occurred) {
                if (is_producer) {
                    int value = dis(gen);
                    queue.enqueue(value);
                    SAFE_COUT("Thread " << thread_id << " enqueued " << value);
                } else {
                    int value;
                    if (queue.dequeue(value)) {
                        SAFE_COUT("Thread " << thread_id << " dequeued " << value);
                    }
                }
                total_operations.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (const std::exception& e) {
            SAFE_COUT("Exception in LockFreeQueue worker " << thread_id << ": " << e.what());
            error_occurred = true;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i % 2 == 0, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(STRESS_TEST_DURATION));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    SAFE_COUT("LockFreeQueue stress test completed. Total operations: " << total_operations.load());

    EXPECT_FALSE(error_occurred) << "An error occurred during the LockFreeQueue stress test";
}

// Updated stress test for LockFreeList
TEST(LockFreeListStressTest, ContinuousInsertRemoveFind) {
    LockFreeList<int> list;
    std::atomic<bool> stop{false};
    std::atomic<long long> total_operations{0};
    std::atomic<bool> error_occurred{false};

    auto worker = [&](int thread_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);
        std::uniform_int_distribution<> op_dis(0, 2);

        try {
            while (!stop && !error_occurred) {
                int value = dis(gen);
                switch (op_dis(gen)) {
                    case 0:
                        list.insert_beginning(value);
                        SAFE_COUT("Thread " << thread_id << " inserted " << value);
                        break;
                    case 1:
                        if (list.remove(value)) {
                            SAFE_COUT("Thread " << thread_id << " removed " << value);
                        }
                        break;
                    case 2:
                        if (list.find(value)) {
                            SAFE_COUT("Thread " << thread_id << " found " << value);
                        }
                        break;
                }
                total_operations.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (const std::exception& e) {
            SAFE_COUT("Exception in LockFreeList worker " << thread_id << ": " << e.what());
            error_occurred = true;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(STRESS_TEST_DURATION));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    SAFE_COUT("LockFreeList stress test completed. Total operations: " << total_operations.load());

    EXPECT_FALSE(error_occurred) << "An error occurred during the LockFreeList stress test";
}


// Updated stress test for LockFreeStack
TEST(LockFreeStackStressTest, ContinuousPushPop) {
    LockFreeStack<int> stack;
    std::atomic<bool> stop{false};
    std::atomic<long long> total_operations{0};
    std::atomic<bool> error_occurred{false};

    auto worker = [&](bool is_producer, int thread_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);

        try {
            while (!stop && !error_occurred) {
                if (is_producer) {
                    int value = dis(gen);
                    stack.push(value);
                    SAFE_COUT("Thread " << thread_id << " pushed " << value);
                } else {
                    auto value = stack.pop();
                    if (value) {
                        SAFE_COUT("Thread " << thread_id << " popped " << *value);
                    }
                }
                total_operations.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (const std::exception& e) {
            SAFE_COUT("Exception in LockFreeStack worker " << thread_id << ": " << e.what());
            error_occurred = true;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i % 2 == 0, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(STRESS_TEST_DURATION));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    SAFE_COUT("LockFreeStack stress test completed. Total operations: " << total_operations.load());

    EXPECT_FALSE(error_occurred) << "An error occurred during the LockFreeStack stress test";
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
