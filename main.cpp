#include "ThreadPool.h"

// --- Example Usage ---
int main() {
    ThreadPool pool(4);
    
    int num1 = 10;
    int num2 = 20;

    // Instead of passing arguments to enqueue, we capture them in the lambda
    pool.enqueue([num1, num2]() {
        std::cout << "Task 1: " << num1 + num2 << "\n";
    });

    pool.enqueue([]() {
        std::cout << "Task 2: Hello from the thread pool!\n";
    });
    // // We don't have futures to wait on, so we add a tiny sleep here just to ensure tasks finish before main() exits and destroys the pool.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return 0;
}