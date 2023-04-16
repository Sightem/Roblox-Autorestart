#pragma once
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>

class Timer
{
public:
    Timer() = default;
    ~Timer() = default;

    Timer(int time) : restartTime(time) {}

    void setRestartTimer(int newTime)
    {
        restartTime.store(newTime, std::memory_order_relaxed);
    }
    
    void terminate()
    {
		stopTimer.store(true, std::memory_order_relaxed);
	}

    bool done() const
    {
		return isDone.load(std::memory_order_relaxed);
	}

    void run()
    {
        isDone.store(false, std::memory_order_relaxed);
        stopTimer.store(false, std::memory_order_relaxed);
        std::cout << "Restarting in ";
        auto start = std::chrono::steady_clock::now();
        while (!stopTimer.load(std::memory_order_relaxed) && std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() < restartTime.load(std::memory_order_relaxed))
        {
            std::string timeleftstr = std::to_string(restartTime.load(std::memory_order_relaxed) - std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count());

            std::cout << timeleftstr << " minutes";

            std::cout << std::string(timeleftstr.length() + 8, '\b');

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        isDone.store(true, std::memory_order_relaxed);
    }

private:
    std::atomic<int> restartTime;
    std::atomic<bool> stopTimer;
    std::atomic<bool> isDone{false};
};