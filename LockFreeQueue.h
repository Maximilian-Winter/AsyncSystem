//
// Created by maxim on 15.09.2024.
//
#pragma once
#include <future>
#include <atomic>
#include <memory>


template <typename T>
class LockFreeQueue {
private:

    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
        Node(T value) : data(std::make_shared<T>(value)), next(nullptr) {}
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
        while (Node* node = head.load(std::memory_order_relaxed)) {
            head.store(node->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            delete node;
        }
    }

    void enqueue(const T& value) {
        Node* new_node = new Node(value);
        Node* old_tail = nullptr;

        while (true) {
            old_tail = tail.load(std::memory_order_acquire);
            Node* tail_next = old_tail->next.load(std::memory_order_acquire);

            if (old_tail == tail.load(std::memory_order_acquire)) {
                if (tail_next == nullptr) {
                    if (old_tail->next.compare_exchange_weak(
                            tail_next, new_node,
                            std::memory_order_release, std::memory_order_relaxed)) {
                        break;
                    }
                } else {
                    tail.compare_exchange_weak(
                        old_tail, tail_next,
                        std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }
        tail.compare_exchange_weak(
            old_tail, new_node,
            std::memory_order_release, std::memory_order_relaxed);
    }

    bool dequeue(T& result) {
        Node* old_head = nullptr;

        while (true) {
            old_head = head.load(std::memory_order_acquire);
            Node* old_tail = tail.load(std::memory_order_acquire);
            Node* head_next = old_head->next.load(std::memory_order_acquire);

            if (old_head == head.load(std::memory_order_acquire)) {
                if (old_head == old_tail) {
                    if (head_next == nullptr) {
                        return false;  // Queue is empty
                    }
                    tail.compare_exchange_weak(
                        old_tail, head_next,
                        std::memory_order_release, std::memory_order_relaxed);
                } else {
                    if (head.compare_exchange_weak(
                            old_head, head_next,
                            std::memory_order_release, std::memory_order_relaxed)) {
                        result = *(head_next->data);
                        delete old_head;
                        return true;
                    }
                }
            }
        }
    }
};
