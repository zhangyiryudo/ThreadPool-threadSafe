#include "ThreadPool_future_variadicTemplate.h"


int main() {
    ThreadPool pool(4);

    // Enqueue some tasks
    auto result1 = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);
    auto result2 = pool.enqueue([](int a, int b) { return a * b; }, 5, 5);

    std::cout << "10 + 20 = " << result1.get() << std::endl;
    std::cout << "5 * 5 = " << result2.get() << std::endl;

    return 0;
}
