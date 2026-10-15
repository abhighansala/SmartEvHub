#ifndef CHARGING_QUEUE_H
#define CHARGING_QUEUE_H

#include <iostream>
#include <string>

const int CHARGING_QUEUE_CAPACITY = 20;

// Educational implementation of a FIFO (First-In-First-Out) Queue
// Uses a simple fixed-size array with FRONT and REAR indices
// NO std::queue used (pure 2nd-year DSA implementation)
// Created by: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
class ChargingQueue {
private:
    std::string items[CHARGING_QUEUE_CAPACITY];
    int frontIndex;
    int rearIndex;
    int currentSize;

public:
    ChargingQueue();

    // Insert element at the REAR of queue
    bool enqueue(const std::string& vehicleId);

    // Remove element from the FRONT of queue
    std::string dequeue();

    // View element at FRONT without removing
    std::string peek() const;

    // Check if queue has no elements
    bool isEmpty() const;

    // Check if queue is full
    bool isFull() const;

    // Print queue contents from FRONT to REAR
    void display() const;

    // Get current number of items
    int getSize() const { return currentSize; }
    int getFrontIndex() const { return frontIndex; }
    int getRearIndex() const { return rearIndex; }

    // Copy queue elements into an array for API output
    int getItems(std::string outArray[], int maxCapacity) const;
};

#endif
