#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <iostream>
#include <regex>
#include "Logger.h"
#include <condition_variable>

class Autorestart
{
public:
	void Start();
	void UnlockRoblox();
	void KillRoblox();
	void FileWatcher(const std::string& path, const std::chrono::seconds& interval, bool& stop, void (*callback)(void*), void* context);
	void SetErrorMsg(const std::string& msg);

	bool ValidateCookies();
	bool RobloxProcessWatcher();
	bool ErrorWactherRoutine();
	bool RobloxWindowWatcher(const size_t& cookiesize);

	int CountWindows(const TCHAR* name);
	
	Autorestart(Logger loggerp)
	{
		loggerp.setPrefix("[Autorestart] ");
		logger = loggerp;
	}

private:
	Logger logger;
	std::condition_variable cv;
	std::condition_variable filewatcher_to_main_signal_cv;
	std::vector<std::string> cookies;

	std::vector<std::regex> regexVec{
		std::regex("join-game-instance POST Body: \\{[^,]*"),
		std::regex("SingleSurfaceAppImpl::returnToLuaApp: App not yet initialized, returning from game"),
		std::regex("Default handler, teleport failed, with state")
	};

	std::string errormsg;

	volatile bool errorwatcher_start = false;
	volatile bool processwatcher_start = false;
	volatile bool windowwatcher_start = false;
};
