#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>

class ThreadPool {
public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            worker_threads.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]() { return !tasks.empty() || terminate; });

                        if (terminate) {
                            return;
                        }

                        {
                            std::unique_lock<std::mutex> active_lock(active_threads_mutex);
                            ++num_active_threads;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();

                    {
                        std::unique_lock<std::mutex> active_lock(active_threads_mutex);
                        --num_active_threads;
                    }

                    {
                        std::unique_lock<std::mutex> pending_lock(pending_tasks_mutex);
                        --num_pending_tasks;
                    }
                }
                });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            terminate = true;
        }
        condition.notify_all();

        for (auto& thread : worker_threads) {
            thread.join();
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            {
                std::unique_lock<std::mutex> pending_lock(pending_tasks_mutex);
                ++num_pending_tasks;
            }

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    size_t activeThreads() {
        std::unique_lock<std::mutex> lock(active_threads_mutex);
        return num_active_threads;
    }

    size_t pendingTasks() {
        std::unique_lock<std::mutex> lock(pending_tasks_mutex);
        return num_pending_tasks;
    }

private:
    std::vector<std::thread> worker_threads;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool terminate = false;

    size_t num_active_threads = 0;
    std::mutex active_threads_mutex;

    size_t num_pending_tasks = 0;
    std::mutex pending_tasks_mutex;
};
