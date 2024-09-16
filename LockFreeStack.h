//
// Created by maxim on 16.09.2024.
//

#pragma once
#include <atomic>
#include <memory>
#include <optional>

template <typename T>
class LockFreeStack {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node(const T& value) : data(std::make_shared<T>(value)), next(nullptr) {}
    };

    std::atomic<Node*> top;

public:
    LockFreeStack() : top(nullptr) {}

    ~LockFreeStack() {
        while (Node* old_top = top.load(std::memory_order_relaxed)) {
            top.store(old_top->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            delete old_top;
        }
    }

    void push(const T& value) {
        Node* new_node = new Node(value);
        Node* old_top = top.load(std::memory_order_relaxed);
        do {
            new_node->next.store(old_top, std::memory_order_relaxed);
        } while (!top.compare_exchange_weak(old_top, new_node,
                                            std::memory_order_release,
                                            std::memory_order_relaxed));
    }

    std::optional<T> pop() {
        Node* old_top = top.load(std::memory_order_relaxed);
        Node* new_top;
        do {
            if (old_top == nullptr) {
                return std::nullopt;  // Stack is empty
            }
            new_top = old_top->next.load(std::memory_order_relaxed);
        } while (!top.compare_exchange_weak(old_top, new_top,
                                            std::memory_order_release,
                                            std::memory_order_relaxed));

        std::optional<T> result = *(old_top->data);
        std::atomic_thread_fence(std::memory_order_acquire);
        delete old_top;
        return result;
    }

    bool is_empty() const {
        return top.load(std::memory_order_relaxed) == nullptr;
    }
};