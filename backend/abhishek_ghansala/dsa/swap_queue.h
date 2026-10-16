#ifndef SWAP_QUEUE_H
#define SWAP_QUEUE_H

#include <iostream>
#include <string>

const int SWAP_QUEUE_CAPACITY = 20;

// Educational implementation of a FIFO (First-In-First-Out) Queue for Battery Swap
// Uses a simple fixed-size array with FRONT and REAR indices
// NO std::queue used (pure 2nd-year DSA implementation)
// Created by: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
class SwapQueue {
private:
    std::string items[SWAP_QUEUE_CAPACITY];
    int frontIndex;
    int rearIndex;
    int currentSize;

public:
    SwapQueue();

    // Insert element at REAR
    bool enqueue(const std::string& vehicleId);

    // Remove element from FRONT
    std::string dequeue();

    // View FRONT element
    std::string peek() const;

    // Check if empty
    bool isEmpty() const;

    // Check if full
    bool isFull() const;

    // Print contents
    void display() const;

    int getSize() const { return currentSize; }
    int getFrontIndex() const { return frontIndex; }
    int getRearIndex() const { return rearIndex; }

    int getItems(std::string outArray[], int maxCapacity) const;
};

#endif
