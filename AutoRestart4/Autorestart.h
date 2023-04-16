#pragma once
#include "Manager.h"
#include "logger.hpp"
#include "json.hpp"
#include "timer.hpp"
#include "taskqueue.hpp"

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

	void init();
    void unlockRoblox();
    bool handleProblematicManagers(const std::vector<size_t>& problematic_managers);
    void enqueueMessages();
    void scheduler();
    void createLogsDirectoryIfNeeded();
    void updateCSRF();

    bool cookiesExists();
	bool readCookies();
	bool validateCookies();

	std::string ensureLogfile();
    std::string generateLogfilePath();

    std::vector<size_t> checkManagersForProblems();

private:
    std::chrono::steady_clock::time_point last_csrf_update;
    std::chrono::milliseconds wait_time_after_restart;
    std::chrono::minutes csrf_update_interval;
    
    ThreadSafeQueue<Message> message_queue;
	std::vector<std::string> cookies;
    std::vector<Manager> managers;

	nlohmann::json* config;
    Timer timer;
    Logger logger;
    
    volatile bool terminate_scheduler;
    bool restart_broken_only;
    bool timer_enabled;

    std::thread scheduler_thread;
    
    //manager stuff
    friend class Manager;
    std::vector<std::string> patterns;
    std::string roblox_exe_path;
    
    int64_t place_id;
    bool launch_vip = false;
    bool launch_sameserver = false;
};