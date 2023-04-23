#pragma once
#include "taskqueue.hpp"
#include "logger.hpp"
#include "timer.hpp"
#include "json.hpp"

#include <condition_variable>
#include <shared_mutex>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <regex>
#include <set>

struct GameJob
{
    std::string JobID;
    int ping;
    int maxPlayers;
    int currentPlayers;
};

class Manager
{
public:

    ~Manager() = default;
    Manager() = default;

    Manager(const std::string& cookie, Logger* logger, const void* autorestart_ptr)
    {
        this->cookie = cookie;
        this->logger = logger;
        this->autorestart_ptr = autorestart_ptr;
    }

    //move constructor
    Manager(Manager&& other) noexcept
    {
        std::swap(cookie, other.cookie);
        std::swap(logger, other.logger);
        std::swap(autorestart_ptr, other.autorestart_ptr);
    }

    Manager& operator=(Manager&& other) noexcept
    {
        if (this != &other)
        {
            std::swap(cookie, other.cookie);
            std::swap(logger, other.logger);
            std::swap(autorestart_ptr, other.autorestart_ptr);
        }
        return *this;
    }
    
    static void SetJobIDString(const std::string& value);
    void HandleMessage(Message& message);
    void LaunchRoblox();
    void Init();

    bool TerminateRoblox();

    static std::string GetJobIDString();
    std::string GetRobloxTicket();
    std::string GetUsername();
    std::string GetCSRF();
    std::string GetSmallestJobID();


    DWORD GetRobloxPID();
    std::set<DWORD> GetRobloxInstances();

private:
    DWORD pid;
    std::string cookie;
    std::string roblox_log_path;
    std::string cookie_username;
    std::string csrf;
    static std::string job_id;
    static std::mutex shared_string_mutex;
    std::mutex regex_mutex;
    
    Logger* logger;
    const void* autorestart_ptr;
};