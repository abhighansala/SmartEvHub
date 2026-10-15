#include "charging_queue.h"

ChargingQueue::ChargingQueue() {
    frontIndex = 0;
    rearIndex = -1;
    currentSize = 0;

    // Pre-populate with two sample waiting EVs matching React prototype
    enqueue("EV-102");
    enqueue("EV-104");
}

bool ChargingQueue::enqueue(const std::string& vehicleId) {
    if (isFull()) {
        std::cout << "[ChargingQueue Overflow] Queue is full! Cannot enqueue " << vehicleId << "\n";
        return false;
    }

    // Circular queue wrap-around
    rearIndex = (rearIndex + 1) % CHARGING_QUEUE_CAPACITY;
    items[rearIndex] = vehicleId;
    currentSize++;

    std::cout << "[ChargingQueue] Enqueued: " << vehicleId 
              << " | FRONT=" << frontIndex << " REAR=" << rearIndex 
              << " SIZE=" << currentSize << "\n";
    return true;
}

std::string ChargingQueue::dequeue() {
    if (isEmpty()) {
        std::cout << "[ChargingQueue Underflow] Queue is empty! Cannot dequeue.\n";
        return "";
    }

    std::string removedItem = items[frontIndex];
    frontIndex = (frontIndex + 1) % CHARGING_QUEUE_CAPACITY;
    currentSize--;

    std::cout << "[ChargingQueue] Dequeued: " << removedItem 
              << " | Remaining size: " << currentSize << "\n";
    return removedItem;
}

std::string ChargingQueue::peek() const {
    if (isEmpty()) {
        return "";
    }
    return items[frontIndex];
}

bool ChargingQueue::isEmpty() const {
    return currentSize == 0;
}

bool ChargingQueue::isFull() const {
    return currentSize == CHARGING_QUEUE_CAPACITY;
}

void ChargingQueue::display() const {
    if (isEmpty()) {
        std::cout << "[ChargingQueue] Queue is EMPTY.\n";
        return;
    }

    std::cout << "[ChargingQueue (FIFO)]: FRONT -> ";
    for (int i = 0; i < currentSize; i++) {
        int idx = (frontIndex + i) % CHARGING_QUEUE_CAPACITY;
        std::cout << "[" << items[idx] << "] ";
    }
    std::cout << "<- REAR\n";
}

int ChargingQueue::getItems(std::string outArray[], int maxCapacity) const {
    int count = 0;
    for (int i = 0; i < currentSize && count < maxCapacity; i++) {
        int idx = (frontIndex + i) % CHARGING_QUEUE_CAPACITY;
        outArray[count] = items[idx];
        count++;
    }
    return count;
}
