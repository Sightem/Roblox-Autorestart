#pragma once
#include "json.hpp"
#include "logger.hpp"
#include "timer.hpp"
#include "taskqueue.hpp"


#include <thread>
#include <shared_mutex>
#include <condition_variable>
#include <fstream>
#include <set>
#include <regex>
#include <Windows.h>

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
    
    void launchRoblox();
    void handleMessage(Message& message);
    bool terminateRoblox();
    void getVIPServerInfo();

    void init();

    std::string getRobloxTicket();
    std::string getUsername();
    std::string getCSRF();
    std::string GetSmallestJobID();
    static std::string getJobIDString();
    static void setJobIDString(const std::string& value);


    DWORD getRobloxPID();
    std::set<DWORD> getRobloxInstances();

private:
    DWORD pid;
    std::string cookie;
    std::string roblox_log_path;
    std::string cookie_username;
    std::string csrf;
    static std::string job_id;
    static std::mutex shared_string_mutex;
    std::mutex regex_mutex;

    //vip server stuff
    std::string link_code; 
    
    Logger* logger;
    const void* autorestart_ptr;
};