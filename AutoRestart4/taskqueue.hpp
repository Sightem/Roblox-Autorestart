#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <optional>
#include <variant>

enum class MessageType 
{
    WAIT,
    CHECK_ROBLOX,
    CHECK_ERROR,
    CHECK_SYNAPSE,
    UPDATE_CSRF
};

struct Message
{
    MessageType type;
    std::variant<std::string, std::chrono::milliseconds, bool> data;
    std::optional<bool> output;
    int priority;

    Message(MessageType type, std::variant<std::string, std::chrono::milliseconds, bool> data, int priority = 0)
        : type(type), data(data), priority(priority) {}

    bool operator<(const Message& other) const 
    {
        return priority < other.priority;
    }
};

template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    void push(const T& value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(value);
        lock.unlock();
        cond_var_.notify_one();
    }

    bool try_pop(T& value) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) 
        {
            return false;
        }
        value = std::move(queue_.top());
        queue_.pop();
        return true;
    }

    void wait_and_pop(T& value) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this]() { return !queue_.empty(); });
        value = std::move(queue_.top());
        queue_.pop();
    }

    bool empty() const 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    T top() 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) 
        {
            throw std::runtime_error("Queue is empty");
        }
        return queue_.top();
    }

    void pop() 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!queue_.empty()) 
        {
            queue_.pop();
        }
    }

private:
    mutable std::mutex mutex_;
    std::priority_queue<T> queue_;
    std::condition_variable cond_var_;
};
