#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include <vector>
#include <regex>
#include <thread>
#include <atomic>
#include <windows.h>
#include <Tlhelp32.h>
#include <tchar.h>

//-- User libs
#include "Autorestart.h"
#include "Roblox.h"
#include "Terminal.h"
#include "Roblox.h"
#include "Request.hpp"
#include "json.hpp"
#include "Logger.h"
#include "Functions.h"
#include "BS_thread_pool.hpp"

using json = nlohmann::json;

std::atomic<bool> Error = false;

void Autorestart::UnlockRoblox()
{
	CreateMutex(NULL, TRUE, "ROBLOX_singletonMutex");
}

void Autorestart::KillRoblox()
{
	clear();
	logger.print(LogLevel::Info, "Killing Roblox");
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	
	if (hSnapShot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapShot, &entry))
		{
			do
			{
				if (!_tcsicmp(entry.szExeFile, "RobloxPlayerBeta.exe"))
				{
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)entry.th32ProcessID);
					if (hProcess != NULL)
					{
						TerminateProcess(hProcess, 9);
						CloseHandle(hProcess);
					}
				}
			} while (Process32Next(hSnapShot, &entry));
		}
		CloseHandle(hSnapShot);
	}
}

bool Autorestart::ValidateCookies()
{
	clear();
	logger.print(LogLevel::Info, "Validating cookies...");

	std::ifstream file("cookies.txt");
	if (file.peek() == std::ifstream::traits_type::eof())
	{
		logger.print(LogLevel::Error, "Cookies.txt is empty");
		wait();
		return false;
	}

	std::vector<std::string> local_cookies;
	std::string line;
	while (std::getline(file, line))
	{
		local_cookies.push_back(line);
	}

	for (auto& cookie : local_cookies)
	{
		long long index = std::distance(local_cookies.begin(), std::find(local_cookies.begin(), local_cookies.end(), cookie));
		
		std::regex pattern("_\\|WARNING:-DO-NOT-SHARE-THIS.--Sharing-this-will-allow-someone-to-log-in-as-you-and-to-steal-your-ROBUX-and-items.\\|_");
		if (!std::regex_search(cookie, pattern))
		{
			logger.print(LogLevel::Error, "A cookie in Cookies.txt is invalid, the first part of the cookie is either corrupted or missing.");
			std::cout << "Invalid cookie on line: " << index + 1 << std::endl;
			wait();
			return false;
		}
		
		if (cookie.find("\"") != std::string::npos)
		{
			logger.print(LogLevel::Error, "A cookie in Cookies.txt is invalid, it contains quotes.");
			std::cout << "Invalid cookie on line: " << index + 1 << std::endl;
			wait();
			return false;
		}
	}

	Request request("https://auth.roblox.com/v1/authentication-ticket");
	request.initalize();

	for (auto& cookie : local_cookies)
	{
		request.set_cookie(".ROBLOSECURITY", cookie);
		request.set_header("Referer", "https://www.roblox.com/");
		Response response = request.post();
		std::string csrfToken = response.headers["x-csrf-token"];

		long long index = std::distance(local_cookies.begin(), std::find(local_cookies.begin(), local_cookies.end(), cookie));
		if (csrfToken.empty())
		{
			std::string errorstr = "A cookie in Cookies.txt is invalid, or may also be expired. Invalid cookie on line: " + std::to_string(index + 1);
			logger.print(LogLevel::Error, errorstr.c_str());
			wait();
			return false;
		}
	}

	cookies = local_cookies;

	local_cookies.size() == 1 ? logger.print(LogLevel::Info, "Cookie is valid!") : logger.print(LogLevel::Info, "Cookies are valid!");

	return true;
}

VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	std::condition_variable* cv = static_cast<std::condition_variable*>(lpParameter);
	cv->notify_all();
}

std::vector<DWORD> GetInstancePIDs()
{
	std::mutex m;
	std::unique_lock<std::mutex> lk(m);

	std::vector<DWORD> pidList;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (strcmp(entry.szExeFile, "RobloxPlayerBeta.exe") == 0)
			{
				HANDLE hProcess = Functions::GetHandleFromPID(entry.th32ProcessID);
				BOOL debugged = FALSE;
				CheckRemoteDebuggerPresent(hProcess, &debugged);
				if (debugged)
				{
					TerminateProcess(hProcess, 9);
				}
				else
				{
					pidList.push_back(entry.th32ProcessID);
				}
			}
		}
	}

	CloseHandle(snapshot);

	return pidList;
}

bool Autorestart::RobloxProcessWatcher()
{
	while(!processwatcher_start)
	{
		std::this_thread::yield();
	}

	std::vector<DWORD> PIDs = GetInstancePIDs();
	std::vector<HANDLE> hWaitObjects;
	std::vector<HANDLE> hProcessHandles;
	for (DWORD pid : PIDs)
	{
		HANDLE hProcess = Functions::GetHandleFromPID(pid);
		hProcessHandles.push_back(hProcess);

		HANDLE hWaitObject = NULL;
		BOOL bSuccess = RegisterWaitForSingleObject(
			&hWaitObject,
			hProcess,
			WaitOrTimerCallback,
			&this->cv,
			INFINITE,
			WT_EXECUTEONLYONCE);
		hWaitObjects.push_back(hWaitObject);
	}

	std::mutex m;
	std::unique_lock<std::mutex> lock(m);
	cv.wait(lock);

	for (HANDLE hWaitObject : hWaitObjects)
	{
		UnregisterWaitEx(hWaitObject, NULL);
	}

	for (HANDLE hProcess : hProcessHandles)
	{
		CloseHandle(hProcess);
	}

	Error.store(true);

	return true;
}

void fileWatcherCallback(void* param)
{
	std::condition_variable* cv = static_cast<std::condition_variable*>(param);

	cv->notify_all();
}

int Autorestart::CountWindows(const TCHAR* name)
{
	int count = 0;
	HWND hwnd = GetTopWindow(NULL);

	while (hwnd != NULL)
	{
		TCHAR title[256];
		GetWindowText(hwnd, title, sizeof(title));

		if (_tcscmp(title, name) == 0)
		{
			count++;
		}

		hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
	}

	return count;
}

bool Autorestart::RobloxWindowWatcher(const size_t& cookiesize)
{
	while (!windowwatcher_start)
	{
		std::this_thread::yield();
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
	
	while (windowwatcher_start)
	{
		if (CountWindows(_T("Roblox")) < cookiesize)
		{
			logger.print(LogLevel::Info, "Roblox window count dropped below cookie count, restarting...");
			Error.store(true);
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	return true;
}

void Autorestart::FileWatcher(const std::string& path, const std::chrono::seconds& interval, bool& stop, void (*callback)(void*), void* context)
{
	std::filesystem::path filePath(path);
	std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(filePath);

	while (!stop)
	{
		std::this_thread::sleep_for(interval);
		if (std::filesystem::last_write_time(filePath) != lastWriteTime)
		{
			lastWriteTime = std::filesystem::last_write_time(filePath);
			std::ifstream file(filePath);
			std::string line((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			bool matchFound = false;

			for (const auto& regex : regexVec) {
				if (std::regex_search(line, regex))
				{
					matchFound = true;
					logger.print(LogLevel::Info, "Found a pattern in a log file");
				}
			}

			if (matchFound) break;
		}
	}

	if (callback) callback(context);
	return;
}

bool Autorestart::ErrorWactherRoutine()
{
	while(!errorwatcher_start)
	{
		std::this_thread::yield();
	}

	std::vector<DWORD> PIDs = GetInstancePIDs();
	std::vector<std::string> Paths = Functions::GetPaths(PIDs);
	
	std::chrono::seconds interval(3);
	//join-game-instance POST Body: {[^,]*
	//SingleSurfaceAppImpl::returnToLuaApp: App not yet initialized, returning from game
	//Default handler, teleport failed, with state


	BS::thread_pool pool(Paths.size());

	bool stop = false;
	for (int i = 0; i < Paths.size(); i++)
	{
		pool.push_task(&Autorestart::FileWatcher, this, Paths[i], interval, std::ref(stop), fileWatcherCallback, &this->cv);
	}

	logger.print(LogLevel::Info, "started file watchers");

	filewatcher_to_main_signal_cv.notify_all();
	
	std::mutex m;
	std::unique_lock<std::mutex> lock(m);
	cv.wait(lock);

	stop = true;
	pool.wait_for_tasks();

	Error.store(true);
	
	return true;
}

void Autorestart::Start()
{
	std::ifstream i("AutoRestartConfig.json");
	json Config;
	i >> Config;

	int RestartTime = Config["Timer"];
	
	std::string placeid = Config["PlaceID"].dump();
	if (placeid.empty())
	{
		logger.print(LogLevel::Error, "PlaceID is empty");
		wait();
		return;
	}
	
	std::string vipurl;
	VIPServer vipserver;
	bool vip = false;
	if (Config["vip"]["Enabled"])
	{
		vipurl = Config["vip"]["url"];

		if (vipurl.empty())
		{
			logger.print(LogLevel::Error, "VIP URL is empty");
			wait();
			return;
		}
		
		vipserver = getVIPServer(vipurl, cookies[0]);
		vip = true;
	}

	while (true)
	{
		UnlockRoblox();

		BS::thread_pool pool;
		std::future<bool> WatcherFuture = pool.submit(&Autorestart::RobloxProcessWatcher, this);
		std::future<bool> ErrorFuture = pool.submit(&Autorestart::ErrorWactherRoutine, this);
		std::future<bool> WindowFuture = pool.submit(&Autorestart::RobloxWindowWatcher, this, cookies.size());
		
		logger.print(LogLevel::Info, "Launching Roblox");

		bool sameserver = Config["SameServer"];
		std::string JobID;

		if (sameserver)
		{
			try
			{
				JobID = GetSmallestJobID(Config["PlaceID"], cookies[0], cookies.size());
			}
			catch (std::exception& e)
			{
				std::string errorstr = "Failed to get JobID, defaulting to normal" + std::string(e.what());
				logger.print(LogLevel::Warning, errorstr.c_str());
				sameserver = false;
			}
		}
		
		for (int i = 0; i < cookies.size(); i++)
		{
			std::string authticket = getRobloxTicket(cookies[i]);

			std::string path = getRobloxPath();

			srand((unsigned int)time(NULL));
			std::string browserTrackerID = std::to_string(rand() % 100000 + 100000);

			std::string unixtime = std::to_string(std::time(nullptr));

			std::string cmd;
			if (vip)
			{
				cmd = '"' + path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestPrivateGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + placeid + "%26accessCode%3D" + vipserver.AccessCode + "%26linkCode%3D" + vipserver.LinkCode + "+browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
			}
			else if (sameserver)
			{
				cmd = '"' + path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestGameJob%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + placeid + "%26gameId%3D" + JobID + "%26isPlayTogetherGame%3Dfalse" + "+browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
			}
			else
			{
				cmd = '"' + path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + placeid + "%26isPlayTogetherGame%3Dfalse+" + "browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
			}

			STARTUPINFOA si = {};
			si.cb = sizeof(si);
			PROCESS_INFORMATION pi = {};
			if (!CreateProcess(&path[0], &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				std::string errorstr = "CreateProcess() failed: " + std::to_string(GetLastError());
				logger.print(LogLevel::Fatal, errorstr.c_str());
			}
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			Functions::_usleep(5000);
		}
		
		logger.print(LogLevel::Info, "Instances Launched");
		Functions::_sleep(10000);
	
		bool forceminimize = Config["ForceMinimize"];

		auto start = std::chrono::steady_clock::now();

		windowwatcher_start = true;
		errorwatcher_start = true;
		processwatcher_start = true;

		std::mutex mutex;
		std::unique_lock<std::mutex> lock(mutex);
		filewatcher_to_main_signal_cv.wait(lock);

		std::cout << "Restarting in ";
		while (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() < RestartTime)
		{
			std::string timeleftstr = std::to_string(RestartTime - std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count());

			std::cout << timeleftstr << " minutes";

			if (forceminimize && FindWindow(NULL, "Roblox"))
			{
				for (int i = 0; i < cookies.size(); i++)
				{
					ShowWindow(FindWindow(NULL, "Roblox"), SW_FORCEMINIMIZE);
				}
			}

			if (Error)
			{
				Error.store(false);
				break;
			}

			if (FindWindow(NULL, "Authentication Failed") || FindWindow(NULL, "Synapse X - Crash Reporter") || FindWindow(NULL, "ROBLOX Crash") || FindWindow(NULL, "Roblox Crash"))
			{
				HWND hWnd = FindWindow(NULL, "Authentication Failed");
				if (hWnd == NULL)	hWnd = FindWindow(NULL, "Synapse X - Crash Reporter");
				if (hWnd == NULL)	hWnd = FindWindow(NULL, "ROBLOX Crash");
				if (hWnd == NULL)	hWnd = FindWindow(NULL, "Roblox Crash");
				if (hWnd != NULL)	SendMessage(hWnd, WM_CLOSE, 0, 0);
				
				break;
			}

			std::cout << std::string(timeleftstr.length() + 8, '\b');
			
			Functions::_usleep(5000);
		}
		cv.notify_all();

		KillRoblox();
		
		logger.print(LogLevel::Info, "waiting for thread to join");
		WindowFuture.wait();
		WatcherFuture.wait();
		ErrorFuture.wait();
		logger.print(LogLevel::Info, "thread joined");
		windowwatcher_start = false;
		errorwatcher_start = false;
		processwatcher_start = false;
		Error.store(false);
		logger.print(LogLevel::Info, "Flags set");

		if (Config["WaitTimeAfterRestart"] != 0)
		{
			logger.print(LogLevel::Info, "Waiting for " + std::to_string(Config["WaitTimeAfterRestart"].get<int>()) + " millieseconds");
			Functions::_sleep(Config["WaitTimeAfterRestart"]);
		}

		logger.print(LogLevel::Info, "Restarting");
	}
}
