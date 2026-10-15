#include "transaction_stack.h"

TransactionStack::TransactionStack() {
    topIndex = -1;

    // Seed stack with recent transactions matching React prototype
    push("TXN-5001");
    push("TXN-5002");
    push("TXN-5003");
}

bool TransactionStack::push(const std::string& txnId) {
    if (isFull()) {
        std::cout << "[TransactionStack Overflow] Stack is full! Cannot push " << txnId << "\n";
        return false;
    }

    topIndex++;
    items[topIndex] = txnId;

    std::cout << "[TransactionStack] Pushed: " << txnId << " | TOP=" << topIndex << "\n";
    return true;
}

std::string TransactionStack::pop() {
    if (isEmpty()) {
        std::cout << "[TransactionStack Underflow] Stack is empty! Cannot pop.\n";
        return "";
    }

    std::string removed = items[topIndex];
    topIndex--;

    std::cout << "[TransactionStack] Popped: " << removed << " | New TOP=" << topIndex << "\n";
    return removed;
}

std::string TransactionStack::peek() const {
    if (isEmpty()) return "";
    return items[topIndex];
}

bool TransactionStack::isEmpty() const {
    return topIndex == -1;
}

bool TransactionStack::isFull() const {
    return topIndex == (TRANSACTION_STACK_CAPACITY - 1);
}

void TransactionStack::display() const {
    if (isEmpty()) {
        std::cout << "[TransactionStack] Stack is EMPTY.\n";
        return;
    }

    std::cout << "[TransactionStack (LIFO)]\n";
    std::cout << "--- TOP ---\n";
    for (int i = topIndex; i >= 0; i--) {
        std::cout << "  [" << items[i] << "]\n";
    }
    std::cout << "--- BOTTOM ---\n";
}

int TransactionStack::getItems(std::string outArray[], int maxCapacity) const {
    int count = 0;
    // Return from TOP down to bottom
    for (int i = topIndex; i >= 0 && count < maxCapacity; i--) {
        outArray[count] = items[i];
        count++;
    }
    return count;
}
