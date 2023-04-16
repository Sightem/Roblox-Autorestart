#pragma once
#include <condition_variable>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <string>
#include <thread>
#include <ctime>
#include <mutex>
#include <queue>

enum LogLevel { DEBUG, INFO, WARNING, ERR };

class Logger 
{
public:
    Logger() 
    {
        loggingThread = std::thread(&Logger::logThread, this);
    }

    Logger(const std::string& filePath) : Logger() 
    {
        setLogFile(filePath);
    }

    ~Logger() 
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            exitFlag = true;
            condition.notify_one();
        }
        loggingThread.join();
        if (logFile.is_open()) 
        {
            logFile.close();
        }
    }

    void setLogFile(const std::string& filePath) 
    {
        std::scoped_lock lock(fileMutex);
        if (logFile.is_open()) 
        {
            logFile.close();
        }
        logFile.open(filePath, std::ofstream::out | std::ofstream::app);
        if (!logFile) 
        {
            throw std::runtime_error("Could not open log file.");
        }
    }

    void dropLogFile() 
    {
        std::scoped_lock lock(fileMutex);
        if (logFile.is_open()) 
        {
            logFile.close();
            logFile.clear();
        }
    }

    void log(const std::string& message, LogLevel level = INFO, const std::string& prefix = "", bool logToFile = false) 
    {
        std::scoped_lock lock(queueMutex);
        logQueue.push({ message, level, prefix, logToFile });
        condition.notify_one();
    }

    bool isDonePrinting() const {
        std::scoped_lock lock(queueMutex);
        return logQueue.empty();
    }

private:
    std::queue<std::tuple<std::string, LogLevel, std::string, bool>> logQueue;
    std::mutex fileMutex;
    std::condition_variable condition;
    std::thread loggingThread;
    std::ofstream logFile;
    mutable std::mutex queueMutex;
    bool exitFlag = false;

    void logThread() 
    {
        while (true) 
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !logQueue.empty() || exitFlag; });

            if (exitFlag && logQueue.empty()) 
            {
                break;
            }

            auto [message, level, prefix, logToFile] = logQueue.front();
            logQueue.pop();
            lock.unlock();

            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            std::ostringstream timestamp;
            timestamp << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

            std::string levelStr;
            switch (level) 
            {
            case DEBUG:
                levelStr = "DEBUG";
                break;
            case INFO:
                levelStr = "INFO";
                break;
            case WARNING:
                levelStr = "WARNING";
                break;
            case ERR:
                levelStr = "ERROR";
                break;
            }

            std::string logLine = "[" + timestamp.str() + "] [" + levelStr + "] " + (prefix.empty() ? "" : "[" + prefix + "] ") + message;

            std::cout << logLine << std::endl;

            if (logToFile) 
            {
                std::scoped_lock fileLock(fileMutex);
                if (logFile.is_open()) 
                {
                    logFile << logLine << std::endl;
                    logFile.flush();
                }
            }
        }
    }
};
