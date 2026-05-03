#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

//C++23, this solution uses modern paradigms like perfect forwarding (std::forward), variadic templates, 
//and std::future/std::packaged_task to ensure that enqueue() can accept any function with any arguments and 
// return a future to the result.

// A. 核心组件：std::future 和 std::packaged_task
// 问题：通常线程只能执行 void 函数。如果你想让线程算一个 1 + 1 并把结果传回主线程，该怎么办？
// 解法：我们使用 std::packaged_task。它能把任何函数包装起来，并关联一个 std::future。主线程拿着这个 future，就像拿着一张“取票凭证”，等子线程算完了，主线程调用 .get() 就能拿到结果。

class ThreadPool {
public:
    // Constructor takes the number of worker threads
    explicit ThreadPool(size_t threads);
    
    // Enqueue a task. Returns a std::future so the caller can get the result later.
    template<class F, class... Args> //F&& f, Args&&... args：这允许你传入任何函数（Lambda、普通函数、成员函数）以及任意数量的参数
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>>;//std::invoke_result_t C++17 的特性，它能在编译阶段“预知”函数执行后的返回类型
        
    // Destructor stops the pool and joins all threads
    ~ThreadPool();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    // Synchronization primitives
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// Constructor: Launches the specified number of worker threads
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i) {
        auto t = [this]()
        {
            for(;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // Wait until there is a task or the pool is stopped
                    this->condition.wait(lock, [this] { 
                        return this->stop || !this->tasks.empty(); 
                    });
                    // If stopped and no more tasks, exit the thread
                    if(this->stop && this->tasks.empty()) {
                        return;
                    }
                    // Pop the next task
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                // Execute the task outside of the lock
                task();
            }
        };
        workers.emplace_back(t);
    }
}

// Enqueue: Adds a new task to the queue
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>> 
{
    using return_type = typename std::invoke_result_t<F, Args...>;

    // Wrap the function in a packaged_task to bridge the gap between 
    // returning a future and storing a void() function in our queue.
    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if(stop) { throw std::runtime_error("enqueue on stopped ThreadPool"); }
        // Wrap the task in a void function and push it to the queue
        tasks.push([task]() { (*task)(); });
    }
    
    // Wake up one sleeping worker thread
    condition.notify_one();
    return res;
}

// Destructor: Flags the stop condition, wakes up all threads, and waits for them to finish
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    
    condition.notify_all();
    for(std::thread &worker : workers) {
        worker.join();
    }
}
