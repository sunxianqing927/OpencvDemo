#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

template <typename T>
class ThreadPool {
public:
    static ThreadPool& getInstance(size_t threadCount = std::thread::hardware_concurrency()) {
        static ThreadPool instance(threadCount);
        return instance;
    }
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    std::future<T> addTask(F&& f, Args&&... args) {
        auto task = std::make_shared<std::packaged_task<T()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<T> result = task->get_future();
        {   
            std::unique_lock<std::mutex> lock(queueMutex);
            if (!isRunning) throw std::runtime_error("ThreadPool is stopped");

            tasks.emplace([task]() { (*task)(); });
        }
        condVar.notify_one();  // 通知线程有任务可取
        return result;
    }

    // 停止线程池并等待所有线程结束
    void stop() {
        {   // 设置停止标志
            std::unique_lock<std::mutex> lock(queueMutex);
            isRunning = false;
        }
        condVar.notify_all(); // 通知所有线程退出

        for (std::thread& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

    ~ThreadPool() {
        stop();
    }

private:
    ThreadPool(size_t threadCount) : isRunning(true) {
        for (size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {   
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condVar.wait(lock, [this] { return !tasks.empty() || !isRunning; });
                        if (!isRunning && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condVar;
    std::atomic<bool> isRunning;
};

