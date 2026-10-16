/**
 * @file test_dsa.cpp
 * @brief Unit Tests for Manual Queue & Stack C++ Data Structures
 * Author: Abhishek Ghansala & Aditya Singh
 */

#include <iostream>
#include <cassert>
#include <string>
#include "../include/dsa/ManualQueue.hpp"
#include "../include/dsa/ManualStack.hpp"

using namespace evhub;

void testQueueBasicOperations() {
    std::cout << "[TEST] Running testQueueBasicOperations...\n";
    ManualQueue<int> q;

    assert(q.isEmpty());
    assert(q.size() == 0);

    int peekVal = 0;
    assert(!q.peek(peekVal)); // Peek on empty queue returns false

    q.enqueue(101);
    assert(!q.isEmpty());
    assert(q.size() == 1);
    assert(q.peek(peekVal) && peekVal == 101);

    q.enqueue(102);
    q.enqueue(103);
    assert(q.size() == 3);

    int outVal = 0;
    // Test FIFO ordering: 101 first
    assert(q.dequeue(outVal) && outVal == 101);
    assert(q.size() == 2);

    assert(q.dequeue(outVal) && outVal == 102);
    assert(q.dequeue(outVal) && outVal == 103);

    assert(q.isEmpty());
    assert(!q.dequeue(outVal)); // Dequeue on empty returns false
    std::cout << "[PASS] testQueueBasicOperations passed.\n";
}

void testStackBasicOperations() {
    std::cout << "[TEST] Running testStackBasicOperations...\n";
    ManualStack<std::string> st;

    assert(st.isEmpty());
    assert(st.size() == 0);

    std::string peekVal;
    assert(!st.peek(peekVal));

    st.push("TX#1");
    st.push("TX#2");
    st.push("TX#3");
    assert(st.size() == 3);

    assert(st.peek(peekVal) && peekVal == "TX#3");

    std::string outVal;
    // Test LIFO ordering: TX#3 popped first
    assert(st.pop(outVal) && outVal == "TX#3");
    assert(st.pop(outVal) && outVal == "TX#2");
    assert(st.pop(outVal) && outVal == "TX#1");

    assert(st.isEmpty());
    assert(!st.pop(outVal));
    std::cout << "[PASS] testStackBasicOperations passed.\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  RUNNING MANUAL DSA UNIT TESTS (C++17) \n";
    std::cout << "========================================\n";

    testQueueBasicOperations();
    testStackBasicOperations();

    std::cout << "All Manual Queue & Stack tests PASSED successfully!\n";
    return 0;
}
