#pragma once
#include <atomic>
#include <memory>

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
        explicit Node(const T& value) : data(std::make_shared<T>(value)), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    LockFreeQueue() {
        Node* dummy = new Node();
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeQueue() {
        while (Node* old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    void enqueue(const T& value) {
        Node* new_node = new Node(value);
        while (true) {
            Node* old_tail = tail.load();
            Node* next = old_tail->next.load();
            if (old_tail == tail.load()) {
                if (next == nullptr) {
                    if (old_tail->next.compare_exchange_weak(next, new_node)) {
                        tail.compare_exchange_strong(old_tail, new_node);
                        return;
                    }
                } else {
                    tail.compare_exchange_weak(old_tail, next);
                }
            }
        }
    }

    bool dequeue(T& result) {
        while (true) {
            Node* old_head = head.load(std::memory_order_acquire);
            Node* old_tail = tail.load(std::memory_order_acquire);
            Node* next = old_head->next.load(std::memory_order_acquire);

            if (old_head == head.load(std::memory_order_acquire)) {
                if (old_head == old_tail) {
                    if (next == nullptr) {
                        return false;  // Queue is empty
                    }
                    tail.compare_exchange_weak(old_tail, next, std::memory_order_release, std::memory_order_relaxed);
                } else {
                    if (next) {
                        result = *(next->data);
                        if (head.compare_exchange_weak(old_head, next, std::memory_order_release, std::memory_order_relaxed)) {
                            // Defer deletion to avoid race condition
                            std::atomic_thread_fence(std::memory_order_acquire);
                            delete old_head;
                            return true;
                        }
                    }
                }
            }
        }
    }
};