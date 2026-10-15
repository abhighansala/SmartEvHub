#ifndef TRANSACTION_STACK_H
#define TRANSACTION_STACK_H

#include <iostream>
#include <string>

const int TRANSACTION_STACK_CAPACITY = 50;

// Educational implementation of a LIFO (Last-In-First-Out) Stack
// Uses a simple fixed-size array with TOP index (-1 when empty)
// NO std::stack used (pure 2nd-year DSA implementation)
// Created by: Abhishek Ghansala (UI + DSA + Final Integration, ID: 2518132)
class TransactionStack {
private:
    std::string items[TRANSACTION_STACK_CAPACITY];
    int topIndex;

public:
    TransactionStack();

    // Push new transaction ID onto TOP of stack
    bool push(const std::string& txnId);

    // Pop (remove) and return transaction ID from TOP of stack
    std::string pop();

    // View TOP element without removing
    std::string peek() const;

    // Check if empty
    bool isEmpty() const;

    // Check if full
    bool isFull() const;

    // Print stack from TOP to BOTTOM
    void display() const;

    int getSize() const { return topIndex + 1; }
    int getTopIndex() const { return topIndex; }

    // Copy items into array (ordered from TOP to bottom for LIFO display)
    int getItems(std::string outArray[], int maxCapacity) const;
};

#endif
