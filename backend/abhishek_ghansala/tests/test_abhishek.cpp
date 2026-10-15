#include <iostream>
#include <cassert>
#include "../dsa/charging_queue.h"
#include "../dsa/swap_queue.h"
#include "../dsa/transaction_stack.h"

// Standalone unit test for Abhishek Ghansala's DSA module
int main() {
    std::cout << "========================================\n";
    std::cout << " Running Tests: Abhishek Ghansala (DSA Queues & Stack)\n";
    std::cout << "========================================\n";

    // 1. Test ChargingQueue FIFO order
    std::cout << "\n--- Testing ChargingQueue (FIFO) ---\n";
    ChargingQueue cQueue;
    // Drain default seeds for test
    while (!cQueue.isEmpty()) {
        cQueue.dequeue();
    }
    assert(cQueue.isEmpty() == true);

    cQueue.enqueue("EV-FIRST");
    cQueue.enqueue("EV-SECOND");
    cQueue.enqueue("EV-THIRD");

    assert(cQueue.peek() == "EV-FIRST");
    assert(cQueue.getSize() == 3);
    std::cout << "[PASS] Enqueue 3 items verified.\n";

    // FIFO: First In must be First Out
    assert(cQueue.dequeue() == "EV-FIRST");
    assert(cQueue.dequeue() == "EV-SECOND");
    assert(cQueue.dequeue() == "EV-THIRD");
    assert(cQueue.isEmpty() == true);
    std::cout << "[PASS] FIFO order verified.\n";

    // Underflow check
    assert(cQueue.dequeue() == "");
    std::cout << "[PASS] Empty queue underflow handled safely.\n";

    // 2. Test SwapQueue
    std::cout << "\n--- Testing SwapQueue (FIFO) ---\n";
    SwapQueue sQueue;
    assert(sQueue.isEmpty() == true);
    sQueue.enqueue("SWAP-1");
    assert(sQueue.peek() == "SWAP-1");
    assert(sQueue.dequeue() == "SWAP-1");
    assert(sQueue.isEmpty() == true);
    std::cout << "[PASS] SwapQueue passed.\n";

    // 3. Test TransactionStack LIFO order
    std::cout << "\n--- Testing TransactionStack (LIFO) ---\n";
    TransactionStack stack;
    while (!stack.isEmpty()) {
        stack.pop();
    }
    assert(stack.isEmpty() == true);

    stack.push("TXN-BOTTOM");
    stack.push("TXN-MIDDLE");
    stack.push("TXN-TOP");

    assert(stack.peek() == "TXN-TOP");
    assert(stack.getSize() == 3);
    std::cout << "[PASS] Push 3 items verified.\n";

    // LIFO: Last In must be First Out
    assert(stack.pop() == "TXN-TOP");
    assert(stack.pop() == "TXN-MIDDLE");
    assert(stack.pop() == "TXN-BOTTOM");
    assert(stack.isEmpty() == true);
    std::cout << "[PASS] LIFO order verified.\n";

    // Underflow check
    assert(stack.pop() == "");
    std::cout << "[PASS] Empty stack underflow handled safely.\n";

    std::cout << "\n>>> ALL TESTS PASSED FOR ABHISHEK GHANSALA! <<<\n";
    return 0;
}
