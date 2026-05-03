#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

//In this bare-bones version, enqueue() simply takes a std::function<void()>. If you need to 
// pass arguments to your tasks, you just capture them in a lambda.

class ThreadPool {
public:
    // Constructor
    explicit ThreadPool(size_t num_threads);
    
    // Destructor
    ~ThreadPool();
    
    // Simple enqueue: takes a void function with no arguments
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
};

inline ThreadPool::ThreadPool(size_t num_threads) {
    for(size_t i = 0; i < num_threads; ++i)
    {
        auto t = [this]()
        {
            while(true) 
            {
                std::function<void()> task;
                {
                    // Lock the queue to check for tasks
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // Sleep until a task is available OR the pool is stopped
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    // Exit thread if stopped and no tasks remain
                    if(this->stop && this->tasks.empty()) {
                        return;
                    }
                    // Grab the task and remove it from the queue
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                // Execute the task outside the lock
                task();
            }
        };
        workers.emplace_back(t);
    }
}

inline void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            // Drop the task (or throw an exception) if pool is stopping
            return; 
        }
        tasks.push(std::move(task));
    }
    // Wake up one worker to handle the new task
    condition.notify_one();
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    // Wake up ALL workers so they can see the 'stop' flag and exit
    condition.notify_all();
    
    for(std::thread &worker : workers) {
        worker.join();
    }
}

