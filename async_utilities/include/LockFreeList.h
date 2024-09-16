//
// Created by maxim on 16.09.2024.
//

#pragma once
#include <atomic>
#include <memory>
#include <optional>

template <typename T>
class LockFreeList {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
        Node(const T& value) : data(std::make_shared<T>(value)), next(nullptr) {}
    };

    std::atomic<Node*> head;

public:
    LockFreeList() : head(nullptr) {}

    ~LockFreeList() {
        Node* current = head.load(std::memory_order_relaxed);
        while (current) {
            Node* next = current->next.load(std::memory_order_relaxed);
            delete current;
            current = next;
        }
    }

    void insert_after(const T& value, const T& after_value) {
        Node* new_node = new Node(value);
        Node* current = head.load(std::memory_order_acquire);

        while (true) {
            while (current && *(current->data) != after_value) {
                current = current->next.load(std::memory_order_acquire);
            }

            if (!current) {
                delete new_node;
                return;  // Value not found, insertion failed
            }

            new_node->next.store(current->next.load(std::memory_order_relaxed), std::memory_order_relaxed);

            if (current->next.compare_exchange_weak(new_node->next.load(std::memory_order_relaxed), new_node,
                                                    std::memory_order_release, std::memory_order_relaxed)) {
                return;  // Insertion successful
            }
        }
    }

    void insert_beginning(const T& value) {
        Node* new_node = new Node(value);
        Node* old_head = head.load(std::memory_order_relaxed);

        do {
            new_node->next.store(old_head, std::memory_order_relaxed);
        } while (!head.compare_exchange_weak(old_head, new_node,
                                             std::memory_order_release, std::memory_order_relaxed));
    }

    bool remove(const T& value) {
        Node* current = head.load(std::memory_order_acquire);
        Node* prev = nullptr;

        while (current && *(current->data) != value) {
            prev = current;
            current = current->next.load(std::memory_order_acquire);
        }

        if (!current) {
            return false;  // Value not found
        }

        Node* next = current->next.load(std::memory_order_acquire);

        if (prev) {
            if (!prev->next.compare_exchange_strong(current, next,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
                return false;  // Node has been removed by another thread
                                                    }
        } else {
            if (!head.compare_exchange_strong(current, next,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {
                return false;  // Head has been changed by another thread
                                              }
        }

        // Defer deletion to avoid race condition
        std::atomic_thread_fence(std::memory_order_acquire);
        delete current;
        return true;
    }

    std::optional<T> find(const T& value) const {
        Node* current = head.load(std::memory_order_acquire);

        while (current) {
            if (*(current->data) == value) {
                return *(current->data);
            }
            current = current->next.load(std::memory_order_acquire);
        }

        return std::nullopt;
    }
};
