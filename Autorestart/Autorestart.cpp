#define CURL_STATICLIB
#include <iostream>
#include <windows.h>
#include <Tlhelp32.h>
#include <chrono>
#include <fstream>
#include <tchar.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <Lmcons.h>
#include <regex>
#include <direct.h>

//-- User libs
#include "Autorestart.h"
#include "Roblox.h"
#include "Terminal.h"
#include "Logger.h"
#include "Request.hpp"

#pragma warning(disable : 4996)

int RestartTime = 0;
void Autorestart::unlockRoblox()
{
	CreateMutex(NULL, TRUE, "ROBLOX_singletonMutex");
}

bool Autorestart::findRoblox()
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

void Autorestart::killRoblox()
{
	clear();
	Log("Killing Roblox", "AutoRestart", true);
	bool found = Autorestart::findRoblox() ? true : false;
	if (found)
	{
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		PROCESSENTRY32 pEntry;

		pEntry.dwSize = sizeof(pEntry);
		BOOL hRes = Process32First(hSnapShot, &pEntry);
		while (hRes)
		{
			if (strcmp(pEntry.szExeFile, "RobloxPlayerBeta.exe") == 0)
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

bool Autorestart::validateCookie()
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

void Autorestart::start(bool forceminimize)
{
	Log("How many minutes before restarting? ", "AutoRestart");
	std::cin >> RestartTime;

	clear();

	std::ifstream infile;
	infile.open("cookies.txt");

	std::vector<std::string> cookies;
	std::string line;
	while (std::getline(infile, line))
	{
		cookies.push_back(line);
	}

	std::ifstream configfile("config.ini");

	bool vip = false;

	while (true)
	{
		std::ifstream file("config.ini");
		std::string workspace;
		std::string placeid;
		std::string vipurl;
		if (file.is_open())
		{
			std::string text;
			int line = 0;

			while (getline(file, text))
			{
				line++;
				switch (line)
				{
				case 1:
					placeid = text;
					break;
				case 2:
					vipurl = text;
					break;
				}
			}
		}

		std::string _placeid = placeid.substr(placeid.find(":") + 1);
		std::string _vipurl = vipurl.substr(vipurl.find(":") + 1);

		if (_placeid.empty())
		{
			std::cout << "placeid is empty" << std::endl;
			wait();
			return;
		}

		std::string LinkCode, accessCode;
		if (!(_vipurl.empty()))
		{
			LinkCode = _vipurl.substr(_vipurl.find("=") + 1);

			Request csrf("https://auth.roblox.com/v1/authentication-ticket");
			csrf.set_cookie(".ROBLOSECURITY", cookies[0]);
			csrf.set_header("Referer", "https://www.roblox.com/");
			csrf.initalize();
			Response res = csrf.post();

			std::string csrfToken = res.headers["x-csrf-token"];

			Request accesscode(_vipurl);
			accesscode.set_cookie(".ROBLOSECURITY", cookies[0]);
			accesscode.set_header("x-csrf-token", csrfToken);
			accesscode.set_header("Referer", "https://www.roblox.com/");
			accesscode.initalize();
			Response res2 = accesscode.get();

			std::regex regex("joinPrivateGame\\(\\d+\\, '(\\w+\\-\\w+\\-\\w+\\-\\w+\\-\\w+)");
			std::smatch match;
			std::regex_search(res2.data, match, regex);
			accessCode = match[1];

			vip = true;
		}
		std::cout << LinkCode << std::endl;
		std::cout << accessCode << std::endl;

		error:
		for (int i = 0; i < cookies.size(); i++)
		{
			unlockRoblox();

			std::string authticket = getRobloxTicket(cookies.at(i));

			std::string path;

			char value[255];
			DWORD BufferSize = 8192;
			RegGetValue(HKEY_CURRENT_USER, "roblox-player\\shell\\open\\command", "", RRF_RT_ANY, NULL, (PVOID)&value, &BufferSize);

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
				cmd = '"' + path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestPrivateGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + _placeid + "%26accessCode%3D" + accessCode + "%26linkCode%3D" + LinkCode + "+browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
			}
			else
			{
				cmd = '"' + path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + _placeid + "%26isPlayTogetherGame%3Dfalse+" + "browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
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
		
		HANDLE hOut;
		COORD coord = { 0, 0 };
		DWORD dwCharsWritten;
		while (std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() <= RestartTime)
		{
			if (forceminimize && FindWindow(NULL, "Roblox"))
			{
				for (int i = 0; i < cookies.size(); i++)
				{
					ShowWindow(FindWindow(NULL, "Roblox"), SW_FORCEMINIMIZE);
				}
			}

			std::string msg = "(" + std::to_string(RestartTime - std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - start).count() + 1) + " minutes)";
			
			hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			FillConsoleOutputCharacter(hOut, ' ', 80 * 25, coord, &dwCharsWritten);
			SetConsoleCursorPosition(hOut, coord);
			
			if (FindWindow(NULL, "Synapse X - Crash Reporter") || FindWindow(NULL, "ROBLOX Crash") || FindWindow(NULL, "Roblox Crash"))
			{ 
				HWND hWnd = FindWindow(NULL, "Synapse X - Crash Reporter");
				if (hWnd == NULL)
				{
					hWnd = FindWindow(NULL, "ROBLOX Crash");
				}
				if (hWnd == NULL)
				{
					hWnd = FindWindow(NULL, "Roblox Crash");
				}
				if (hWnd != NULL)
				{
					SendMessage(hWnd, WM_CLOSE, 0, 0);
				}

				killRoblox();
				_usleep(5000);
				goto error;
			}

			Log(msg, "AutoRestart");

			_usleep(5000);
		}
		
		killRoblox();
		_sleep(5000);
	}
}
