#pragma once
#include "taskqueue.hpp"
#include "logger.hpp"
#include "Manager.h"
#include "timer.hpp"
#include "json.hpp"

#include <Windows.h>
#include <atomic>

class Autorestart
{
public:
    Autorestart()
    {
        last_csrf_update = std::chrono::steady_clock::now();
        csrf_update_interval = std::chrono::minutes(10);
    }

    Autorestart(nlohmann::json* config) : Autorestart()
    {
		this->config = config;
	}

	~Autorestart() = default;

    bool HandleProblematicManagers(const std::vector<size_t>& problematic_managers);
    void CreateLogsDirectoryIfNeeded();
    void EnqueueMessages();
    void UnlockRoblox();
    void UpdateCSRF();
    void Scheduler();
	void Init();

    bool CookiesExists();
	bool ReadCookies();
	bool ValidateCookies();

	std::string EnsureLogfile();
    std::string GenerateLogFilePath();

    std::vector<size_t> CheckManagersForProblems();

private:
    std::chrono::steady_clock::time_point last_csrf_update;
    std::chrono::milliseconds wait_time_after_restart;
    std::chrono::minutes csrf_update_interval;
    
    ThreadSafeQueue<Message> message_queue;
	std::vector<std::string> cookies;
    std::vector<Manager> managers;

	nlohmann::json* config;
    Logger logger;
    Timer timer;
    
    volatile bool terminate_scheduler;
    bool restart_broken_only;
    bool timer_enabled;
    bool patterns_empty;

    int time;

    std::thread scheduler_thread;
    
    //manager stuff
    friend class Manager;
    std::vector<std::string> patterns;
    std::string roblox_exe_path;
    std::string link_code;
    
    int64_t place_id;
    bool launch_vip = false;
    bool launch_sameserver = false;
};