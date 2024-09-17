#pragma once
#include <atomic>
#include <memory>
#include <optional>


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
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        while (Node* old_head = head.load(std::memory_order_relaxed)) {
            head.store(old_head->next, std::memory_order_relaxed);
            delete old_head;
        }
    }

    void enqueue(T value) {
        Node* new_node = new Node(std::move(value));
        while (true) {
            Node* old_tail = tail.load(std::memory_order_acquire);
            Node* next = old_tail->next.load(std::memory_order_acquire);
            if (old_tail == tail.load(std::memory_order_acquire)) {
                if (next == nullptr) {
                    if (old_tail->next.compare_exchange_weak(next, new_node,
                                                             std::memory_order_release,
                                                             std::memory_order_relaxed)) {
                        tail.compare_exchange_strong(old_tail, new_node,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed);
                        return;
                    }
                } else {
                    tail.compare_exchange_weak(old_tail, next,
                                               std::memory_order_release,
                                               std::memory_order_relaxed);
                }
            }
        }
    }

    std::optional<T> dequeue() {
        while (true) {
            Node* old_head = head.load(std::memory_order_acquire);
            Node* old_tail = tail.load(std::memory_order_acquire);
            Node* next = old_head->next.load(std::memory_order_acquire);

            if (old_head == head.load(std::memory_order_acquire)) {
                if (old_head == old_tail) {
                    if (next == nullptr) {
                        return std::nullopt;  // Queue is empty
                    }
                    tail.compare_exchange_weak(old_tail, next,
                                               std::memory_order_release,
                                               std::memory_order_relaxed);
                } else {
                    if (next && next->data) {
                        T result = std::move(*next->data);
                        if (head.compare_exchange_weak(old_head, next,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed)) {
                            delete old_head;
                            return result;
                        }
                    } else {
                        // Skip this node if it's invalid
                        head.compare_exchange_weak(old_head, next,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed);
                    }
                }
            }
        }
    }

    bool is_empty() const {
        Node* front = head.load(std::memory_order_acquire);
        return front->next.load(std::memory_order_acquire) == nullptr;
    }
};
