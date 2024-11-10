#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
   
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Enqueue a task for the thread pool to execute
    template <typename F, typename... Args>
    void enqueue(F&& function, Args&&... args) {
        auto task = std::bind(std::forward<F>(function), std::forward<Args>(args)...);

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace([task]() { task(); });
        }

        condition.notify_one();
    }
    void stopThread();
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
   
};


#endif