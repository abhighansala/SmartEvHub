#pragma once

#include <iostream>
#include <mutex>
#include <vector>
#include <stdexcept>

namespace evhub {

/**
 * @brief Manual Linked LIFO Stack Implementation.
 * Used for transaction audit trail and recent activity tracking.
 * Demonstrates manual pointer-based dynamic data structures for B.Tech CSE requirements.
 */
template <typename T>
class ManualStack {
private:
    struct Node {
        T data;
        Node* next;
        explicit Node(const T& val) : data(val), next(nullptr) {}
    };

    Node* top_;
    size_t count_;
    mutable std::mutex mtx_;

public:
    ManualStack() : top_(nullptr), count_(0) {}

    // Destructor - RAII cleanup of nodes
    ~ManualStack() {
        clear();
    }

    ManualStack(const ManualStack&) = delete;
    ManualStack& operator=(const ManualStack&) = delete;

    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        Node* newNode = new Node(item);
        newNode->next = top_;
        top_ = newNode;
        count_++;
    }

    bool pop(T& outItem) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (isEmptyInternal()) {
            return false;
        }
        Node* temp = top_;
        outItem = temp->data;
        top_ = top_->next;
        delete temp;
        count_--;
        return true;
    }

    bool peek(T& outItem) const {
        std::lock_guard<std::mutex> lock(mtx_);
        if (isEmptyInternal()) {
            return false;
        }
        outItem = top_->data;
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
        while (top_ != nullptr) {
            Node* temp = top_;
            top_ = top_->next;
            delete temp;
        }
        count_ = 0;
    }

    // Top-to-bottom vector representation for REST API audit listing
    std::vector<T> toVector() const {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<T> result;
        result.reserve(count_);
        Node* curr = top_;
        while (curr != nullptr) {
            result.push_back(curr->data);
            curr = curr->next;
        }
        return result;
    }

    void display(std::ostream& os = std::cout) const {
        std::lock_guard<std::mutex> lock(mtx_);
        os << "[TOP]\n";
        Node* curr = top_;
        while (curr != nullptr) {
            os << "  | " << curr->data << "\n";
            curr = curr->next;
        }
        os << "[BOTTOM]\n";
    }

private:
    bool isEmptyInternal() const {
        return top_ == nullptr;
    }
};

} // namespace evhub
