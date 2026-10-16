#pragma once

#include <iostream>
#include <mutex>
#include <vector>
#include <stdexcept>

namespace evhub {

/**
 * @brief Manual Linked FIFO Queue Implementation.
 * Written by UI/DSA Owner (Abhishek Ghansala) & integrated with Backend (Aditya Singh).
 * Fulfills second-year B.Tech academic requirement for custom data structure implementation.
 */
template <typename T>
class ManualQueue {
private:
    struct Node {
        T data;
        Node* next;
        explicit Node(const T& val) : data(val), next(nullptr) {}
    };

    Node* front_;
    Node* rear_;
    size_t count_;
    mutable std::mutex mtx_;

public:
    ManualQueue() : front_(nullptr), rear_(nullptr), count_(0) {}

    // Destructor - RAII cleanup of linked nodes
    ~ManualQueue() {
        clear();
    }

    // Disable copy to prevent double deletion of heap nodes
    ManualQueue(const ManualQueue&) = delete;
    ManualQueue& operator=(const ManualQueue&) = delete;

    void enqueue(const T& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        Node* newNode = new Node(item);
        if (rear_ == nullptr) {
            front_ = rear_ = newNode;
        } else {
            rear_->next = newNode;
            rear_ = newNode;
        }
        count_++;
    }

    bool dequeue(T& outItem) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (isEmptyInternal()) {
            return false;
        }
        Node* temp = front_;
        outItem = temp->data;
        front_ = front_->next;
        if (front_ == nullptr) {
            rear_ = nullptr;
        }
        delete temp;
        count_--;
        return true;
    }

    bool peek(T& outItem) const {
        std::lock_guard<std::mutex> lock(mtx_);
        if (isEmptyInternal()) {
            return false;
        }
        outItem = front_->data;
        return true;
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return isEmptyInternal();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return count_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        while (front_ != nullptr) {
            Node* temp = front_;
            front_ = front_->next;
            delete temp;
        }
        rear_ = nullptr;
        count_ = 0;
    }

    // Returns linear snapshot for REST API serialization
    std::vector<T> toVector() const {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<T> result;
        result.reserve(count_);
        Node* curr = front_;
        while (curr != nullptr) {
            result.push_back(curr->data);
            curr = curr->next;
        }
        return result;
    }

    void display(std::ostream& os = std::cout) const {
        std::lock_guard<std::mutex> lock(mtx_);
        os << "[FRONT] ";
        Node* curr = front_;
        while (curr != nullptr) {
            os << "-> (" << curr->data << ") ";
            curr = curr->next;
        }
        os << "[REAR]\n";
    }

private:
    bool isEmptyInternal() const {
        return front_ == nullptr;
    }
};

} // namespace evhub
