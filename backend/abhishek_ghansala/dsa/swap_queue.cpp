#include "swap_queue.h"

SwapQueue::SwapQueue() {
    frontIndex = 0;
    rearIndex = -1;
    currentSize = 0;
}

bool SwapQueue::enqueue(const std::string& vehicleId) {
    if (isFull()) {
        std::cout << "[SwapQueue Overflow] Queue is full!\n";
        return false;
    }

    rearIndex = (rearIndex + 1) % SWAP_QUEUE_CAPACITY;
    items[rearIndex] = vehicleId;
    currentSize++;

    std::cout << "[SwapQueue] Enqueued: " << vehicleId << " | SIZE=" << currentSize << "\n";
    return true;
}

std::string SwapQueue::dequeue() {
    if (isEmpty()) {
        std::cout << "[SwapQueue Underflow] Queue is empty!\n";
        return "";
    }

    std::string removedItem = items[frontIndex];
    frontIndex = (frontIndex + 1) % SWAP_QUEUE_CAPACITY;
    currentSize--;

    std::cout << "[SwapQueue] Dequeued: " << removedItem << "\n";
    return removedItem;
}

std::string SwapQueue::peek() const {
    if (isEmpty()) return "";
    return items[frontIndex];
}

bool SwapQueue::isEmpty() const {
    return currentSize == 0;
}

bool SwapQueue::isFull() const {
    return currentSize == SWAP_QUEUE_CAPACITY;
}

void SwapQueue::display() const {
    if (isEmpty()) {
        std::cout << "[SwapQueue] Empty.\n";
        return;
    }

    std::cout << "[SwapQueue (FIFO)]: FRONT -> ";
    for (int i = 0; i < currentSize; i++) {
        int idx = (frontIndex + i) % SWAP_QUEUE_CAPACITY;
        std::cout << "[" << items[idx] << "] ";
    }
    std::cout << "<- REAR\n";
}

int SwapQueue::getItems(std::string outArray[], int maxCapacity) const {
    int count = 0;
    for (int i = 0; i < currentSize && count < maxCapacity; i++) {
        int idx = (frontIndex + i) % SWAP_QUEUE_CAPACITY;
        outArray[count] = items[idx];
        count++;
    }
    return count;
}
