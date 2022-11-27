#define CURL_STATICLIB
#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include <vector>
#include <filesystem>
#include <Lmcons.h>
#include <regex>
#include <thread>
#include <atomic>
#include <direct.h>
#include <windows.h>
#include <Tlhelp32.h>
#include <tchar.h>
#include <WtsApi32.h>

//-- User libs
#include "Autorestart.h"
#include "Roblox.h"
#include "Terminal.h"
#include "Logger.h"
#include "Request.hpp"
#include "json.hpp"
#pragma comment(lib, "Wtsapi32.lib" )

using json = nlohmann::json;


volatile int CookieCount = 0;
volatile bool Ready = false;
std::atomic<bool> Error = false;

void Autorestart::UnlockRoblox()
{
	CreateMutex(NULL, TRUE, "ROBLOX_singletonMutex");
}

bool Autorestart::FindRoblox()
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (!Process32First(snapshot, &entry))
	{
		CloseHandle(snapshot);
		return false;
	}

	do
	{
		if (!_tcsicmp(entry.szExeFile, "RobloxPlayerBeta.exe"))
		{
			CloseHandle(snapshot);
			return true;
		}
	} while (Process32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return false;
}

void Autorestart::KillRoblox()
{
	clear();
	Log("Killing Roblox", "AutoRestart", true);
	bool found = Autorestart::FindRoblox();
	if (found)
	{
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		PROCESSENTRY32 pEntry;

		pEntry.dwSize = sizeof(pEntry);
		BOOL hRes = Process32First(hSnapShot, &pEntry);
		while (hRes)
		{
			if (!strcmp(pEntry.szExeFile, "RobloxPlayerBeta.exe"))
			{
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);

				if (hProcess != NULL)
				{
					TerminateProcess(hProcess, 9);
					CloseHandle(hProcess);
				}
			}
			hRes = Process32Next(hSnapShot, &pEntry);
		}
		CloseHandle(hSnapShot);
	}
}

void Autorestart::_usleep(int microseconds)
{
	std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}
void Autorestart::_sleep(int miliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(miliseconds));
}

bool Autorestart::ValidateCookies()
{
	clear();
	Log("Validating cookies...", "AutoRestart", true);

	std::ifstream file("cookies.txt");
	if (file.peek() == std::ifstream::traits_type::eof())
	{
		Log("Cookies.txt is empty", "AutoRestart", true);
		wait();
		return false;
	}

	std::vector<std::string> cookies;
	std::string line;
	while (std::getline(file, line))
	{
		cookies.push_back(line);
	}
	CookieCount = cookies.size();

	for (auto& cookie : cookies)
	{
		long long index = std::distance(cookies.begin(), std::find(cookies.begin(), cookies.end(), cookie));

		if (cookie.find("_|WARNING:") == std::string::npos || cookie.find("ROBUX") == std::string::npos)
		{
			Log("A cookie in Cookies.txt is invalid, the first part of the cookie is either corrupted or missing.", "AutoRestart", true);
			std::cout << "Invalid cookie on line: " << index + 1 << std::endl;
			wait();
			return false;
		}
		if (cookie.find("\"") != std::string::npos)
		{
			Log("A cookie in Cookies.txt is invalid, it contains quotes.", "AutoRestart", true);
			std::cout << "Invalid cookie on line: " << index + 1 << std::endl;
			wait();
			return false;
		}
	}

	Request request("https://auth.roblox.com/v1/authentication-ticket");
	request.initalize();

	for (auto& cookie : cookies)
	{
		request.set_cookie(".ROBLOSECURITY", cookie);
		request.set_header("Referer", "https://www.roblox.com/");
		Response response = request.post();
		std::string csrfToken = response.headers["x-csrf-token"];

		long long index = std::distance(cookies.begin(), std::find(cookies.begin(), cookies.end(), cookie));
		if (csrfToken.empty())
		{
			Log("A cookie in Cookies.txt is invalid, or may also be expired", "AutoRestart", true);
			std::cout << "Invalid cookie on line: " << index + 1 << std::endl;
			wait();
			return false;
		}
	}
	Log("ze cookie(s) are valid!", "AutoRestart", true);
	return true;
}

HANDLE Autorestart::GetHandleFromPID(DWORD pid)
{
	return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
}

int Autorestart::GetInstanceCount()
{
	int count = 0;
	WTS_PROCESS_INFO* pWPIs = NULL;
	DWORD dwProcCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount))
	{
		for (DWORD i = 0; i < dwProcCount; i++)
		{
			std::string name = std::string(pWPIs[i].pProcessName);
			if (name.find("RobloxPlayerBeta") != std::string::npos)
			{
				BOOL debugged = FALSE;
				CheckRemoteDebuggerPresent(GetHandleFromPID(pWPIs[i].ProcessId), &debugged);
				if (!debugged) count++;
			}
		}
	}
	if (pWPIs) { WTSFreeMemory(pWPIs); pWPIs = NULL; }
	return count;
}

void Autorestart::WorkspaceWatcher()
{ 
	std::ifstream i("AutoRestartConfig.json");
	json Config;
	i >> Config;
	
	std::string Directory = Config["WorkspaceInteraction"]["Path"];
	std::string FileName = Config["WorkspaceInteraction"]["FileName"];

	while (true)
	{
		while (!Ready)
		{
			std::this_thread::yield();
		}

		if (std::filesystem::exists(Directory + FileName) && Ready)
		{
			std::string Path = Directory + "\\" + FileName;
			LPCSTR PathLPCSTR = Path.c_str();
			DeleteFile(PathLPCSTR);
			
			Error.store(true);
		}

		Autorestart::_sleep(500);
	}
}


void Autorestart::RobloxProcessWatcher()
{
	while (true)
	{
		int stroke = 0;

		while (!Ready) 
		{
			std::this_thread::yield();
		}

		int tries = 0;
		if (GetInstanceCount() < CookieCount)
		{
			while (tries < 30)
			{
				if (GetInstanceCount() == CookieCount)
				{
					break;
				}
				tries++;
				Autorestart::_sleep(1000);
			}
		}

		while (true)
		{
			if (GetInstanceCount() < CookieCount && Ready)
			{
				stroke++;
				if (stroke == 30)
				{
					Error.store(true);
					break;
				}
			}
			Autorestart::_sleep(1000);
		}
	}
}

void Autorestart::Start()
{
	std::ifstream i("AutoRestartConfig.json");
	json Config;
	i >> Config;

	int RestartTime = Config["Timer"];
	
	std::ifstream infile;
	infile.open("cookies.txt");

	std::vector<std::string> cookies;
	std::string line;
	while (std::getline(infile, line))
	{
		cookies.push_back(line);
	}

	std::string placeid = Config["PlaceID"].dump();
	std::string vipurl;
	bool vip = false;
	
	if (Config["vip"]["Enabled"]) vipurl = Config["vip"]["url"];

	if (placeid.empty())
	{
		std::cout << "PlaceID is empty" << std::endl;
		wait();
		return;
	}

	std::string LinkCode, AccessCode;
	if (Config["vip"]["Enabled"] && !vipurl.empty())
	{
		LinkCode = vipurl.substr(vipurl.find("=") + 1);

		Request csrf("https://auth.roblox.com/v1/authentication-ticket");
		csrf.set_cookie(".ROBLOSECURITY", cookies[0]);
		csrf.set_header("Referer", "https://www.roblox.com/");
		csrf.initalize();
		Response res = csrf.post();

		std::string csrfToken = res.headers["x-csrf-token"];

		Request accesscode(vipurl);
		accesscode.set_cookie(".ROBLOSECURITY", cookies[0]);
		accesscode.set_header("x-csrf-token", csrfToken);
		accesscode.set_header("Referer", "https://www.roblox.com/");
		accesscode.initalize();
		Response res2 = accesscode.get();

		std::regex regex("joinPrivateGame\\(\\d+\\, '(\\w+\\-\\w+\\-\\w+\\-\\w+\\-\\w+)");
		std::smatch match;
		std::regex_search(res2.data, match, regex);
		AccessCode = match[1];

		vip = true;
	}

	std::thread RobloxProcessWatcherThread;
	std::thread WorkspaceWatcherThread;
	if (Config["Watchdog"]) RobloxProcessWatcherThread = std::thread(&Autorestart::RobloxProcessWatcher, this);
	if (Config["WorkspaceInteraction"]["Enabled"]) WorkspaceWatcherThread = std::thread(&Autorestart::WorkspaceWatcher, this);
	
	while (true)
	{
		Log("Launching Roblox", "AutoRestart", true);
		
		for (int i = 0; i < cookies.size(); i++)
		{
			UnlockRoblox();

			std::string authticket = getRobloxTicket(cookies.at(i));

			std::string path;

			char value[255];
			DWORD BufferSize = 8192;
			RegGetValue(HKEY_CLASSES_ROOT, "roblox-player\\shell\\open\\command", "", RRF_RT_ANY, NULL, (PVOID)&value, &BufferSize);
			path = value;
			path = path.substr(1, path.length() - 5);

			srand((unsigned int)time(NULL));

			std::string randomnumber = std::to_string(rand() % 100000 + 100000);
			std::string randomnumber2 = std::to_string(rand() % 100000 + 100000);
			std::string unixtime = std::to_string(std::time(nullptr));
			std::string browserTrackerID = randomnumber + randomnumber2;

			std::string cmd;
			if (vip)
			{
				cmd = '"' + path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestPrivateGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + placeid + "%26accessCode%3D" + AccessCode + "%26linkCode%3D" + LinkCode + "+browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
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
				Log("CreateProcess() failed: " + GetLastError(), LOG_FATAL);
				return;
			}
			WaitForSingleObject(pi.hProcess, INFINITE);
			this->robloxProcesses.push_back(pi);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			_usleep(10000);
		}

		auto start = std::chrono::steady_clock::now();
		
		Ready = true;
		
		Log("Restarting in ", "AutoRestart", true);
		while (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() <= RestartTime)
		{
			std::string timeleftstr = std::to_string(RestartTime - std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() + 1);

			std::cout << timeleftstr << " minutes";

			if (Config["ForceMinimize"] && FindWindow(NULL, "Roblox"))
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
				if (hWnd == NULL)				hWnd = FindWindow(NULL, "Synapse X - Crash Reporter");
				if (hWnd == NULL)				hWnd = FindWindow(NULL, "ROBLOX Crash");
				if (hWnd == NULL)				hWnd = FindWindow(NULL, "Roblox Crash");
				if (hWnd != NULL)				SendMessage(hWnd, WM_CLOSE, 0, 0);
				
				break;
			}

			std::cout << std::string(timeleftstr.length() + 8, '\b');
			_usleep(5000);
		}
		Ready = false;
		Error.store(false);
		KillRoblox();
		
		while (Autorestart::FindRoblox())
		{
			std::this_thread::yield();
		}
	}
}
