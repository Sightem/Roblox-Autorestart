#define CURL_STATICLIB
#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <filesystem>

//-- User libs
#include "Request.hpp"
#include "Autorestart.h"
#include "Roblox.h"
#include "Terminal.h"
#include "json.hpp"
#include "Logger.h"

//-- External libs
#ifdef _DEBUG
#pragma comment (lib, "curl/libcurl_a_debug.lib")
#else
#pragma comment (lib,"curl/libcurl_a.lib")
#endif

#pragma comment (lib, "Normaliz.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Wldap32.lib")
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "User32.lib")

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
namespace fs = std::filesystem;

const ordered_json DEFAULT_CONFIG =
{
	{"Timer", 10},
	{"PlaceID", 1},
	{"SameServer", true},
	{"TopMost", true},
	{"ForceMinimize", false},
	{"Resizablewindow", false},
	{"vip", {
		{"Enabled", false},
		{"url", ""}
		}
	},
	{"WaitTimeAfterRestart", 10000},
	{"Watchdog", true}
};


void CreateConfig();
void CreateCookies();
void Compatibility();
void PatchConfig();
std::string return_current_time_and_date();

Logger logger({ &std::cout });
std::ofstream logFile;

int main(int argc, char* argv[])
{
	SetConsoleTitle("Roblox Autorestart v6dbg");

	//-- Read launch arguments
	bool compat = false;
	if (argc > 1)
	{
		for (int i = 0; i < argc; i++)
		{
			if (strcmp(argv[i], "compat") == 0) { compat = true; }
		}
	}

	//-- Logging setup
	if (std::filesystem::exists("logs") == false) 
	{ 
		std::filesystem::create_directory("logs"); 
	}

	std::string filename = "logs/AutoRestart-" + return_current_time_and_date() + ".txt";
	std::ofstream file(filename);
	if (file.is_open()) 
	{
		file.close();
	}
	else 
	{
		std::cout << "Error creating file " << filename << " in 'logs' directory." << std::endl;
		wait();
		return 1;
	}

	logFile = std::ofstream(filename, std::ios::app);
	logger.addStream(&logFile);
	logger.setFormatString("[%Y-%m-%d %H:%M:%S]");
	logger.setPrefix("[AutoRestart] [Main] ");

	//-- check if AutoRestartConfig.json exists
	if (!fs::exists("AutoRestartConfig.json"))
	{
		logger.print(LogLevel::Info, "Config file not found, creating");
		CreateConfig();
		logger.print(LogLevel::Info, "Config file created");
		wait();
		return 1;
	}

	//-- check if cookies.txt exists
	if (!fs::exists("cookies.txt"))
	{
		CreateCookies();
	}

	//-- Config parsing
	std::ifstream i("AutoRestartConfig.json");
	json Config;
	try
	{
		i >> Config;
	}
	catch (std::exception& e)
	{
		std::cout << "Error parsing config file: " << e.what() << std::endl;
		wait();
		return 1;
	}

	//-- Check if config is up to date
	PatchConfig();
	
	//-- Top Most
	if (Config["TopMost"])
	{
		::SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		::ShowWindow(GetConsoleWindow(), SW_NORMAL);
	}

	//-- Window size lock
	if (Config["Resizablewindow"])
	{
		SetWindowLong(GetConsoleWindow(), GWL_STYLE, GetWindowLong(GetConsoleWindow(), GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	}

	if (compat)
	{
		Compatibility();
	}

	Autorestart autorestart(logger);
	if (autorestart.ValidateCookies())
	{
		autorestart.Start();
	}
	else
	{
		logger.print(LogLevel::Error, "Cookie validation failed.");
		wait();
		return 0;
	}
}

void Compatibility()
{
	logger.print(LogLevel::Info, "Running in compatibility mode");

	if (std::filesystem::exists("AccountData.json"))
	{
		bool cookiesexist = std::filesystem::exists("cookies.txt");

		//this is a forked version of RAMDecrypt, which removes the unneeded parts
		system("curl -LJOs https://github.com/Sightem/RAMDecrypt/releases/download/1.1/ramdecr.exe");

		json data;
		try
		{
			data = json::parse(std::ifstream("AccountData.json"));
		}
		catch (std::exception& e)
		{
			system("ramdecr.exe AccountData.json");
			data = json::parse(std::ifstream("AccountData.json"));
		}
		
		if (cookiesexist)
		{
			std::vector<std::string> cookies;
			std::ifstream cookiefile("cookies.txt");
			std::string line;
			while (std::getline(cookiefile, line))
			{
				cookies.push_back(line);
			}
			
			std::vector<std::string> usernames = GetUsernames(cookies);

			logger.print({ &std::cout }, LogLevel::Warning, "Cookies already exist for the following accounts: ");
			logger.print({ &logFile }, LogLevel::Warning, "Cookies already present, presenting cookies to the user");
			for (int i = 0; i < usernames.size(); i++)
			{
				if (i == usernames.size() - 1)
				{
					std::cout << usernames[i] << std::endl;
				}
				else
				{
					std::cout << usernames[i] << ", ";
				}
			}
		}
		
		logger.print({ &std::cout }, LogLevel::Info, "Accounts to be imported: ");
		for (int i = 0; i < data.size(); i++)
		{
			std::cout << "     " << i + 1 << ": " << data[i]["Username"] << std::endl;
		}
		std::cout << "Select an account to parse (separate with commas): ";
		
		std::string input;
		std::vector<int> choices;

		std::getline(std::cin, input);
		std::stringstream ss(input);
		int temp;
		while (ss >> temp)
		{
			choices.push_back(temp);
			if (ss.peek() == ',')
				ss.ignore();
		}

		if (cookiesexist)
		{
			std::ofstream o("cookies.txt", std::ios::app);
			for (int i = 0; i < choices.size(); i++)
			{
				std::string cookie = data[choices[i] - 1]["SecurityToken"];
				cookie.erase(std::remove(cookie.begin(), cookie.end(), '\"'), cookie.end());
				o << cookie << std::endl;
			}
		}
		else
		{
			std::ofstream o("cookies.txt");
			for (int i = 0; i < choices.size(); i++)
			{
				std::string cookie = data[choices[i] - 1]["SecurityToken"];
				cookie.erase(std::remove(cookie.begin(), cookie.end(), '\"'), cookie.end());
				o << cookie << std::endl;
			}
		}
	}
	else
	{
		std::cout << "AccountData.json not found, please place it in the same directory as this executable" << std::endl;
		wait();
	}

	logger.print({ &logFile }, LogLevel::Info, "Import complete.");
}

void CreateCookies()
{
	std::ofstream file("cookies.txt");
	file << "";
	file.close();
}

void CreateConfig()
{
	std::ofstream config;
	config.open("AutoRestartConfig.json");
	
	config << DEFAULT_CONFIG.dump(4);
	
	config.close();
}

void PatchConfig()
{
	std::cout << "patching\n";

	std::ifstream i("AutoRestartConfig.json");

	ordered_json localConfig;
	
	try
	{
		localConfig = ordered_json::parse(i);
	}
	catch (std::exception& e)
	{
		std::cout << "Error parsing config file: " << e.what() << std::endl;
		wait();
		return;
	}

	ordered_json patch = ordered_json::diff(localConfig, DEFAULT_CONFIG);

	for (int i = 0; i < patch.size(); i++)
	{
		if (patch[i].contains("op") && patch[i]["op"] == "replace")
		{
			patch.erase(i);
			i--;
		}
	}

	ordered_json newConfig = localConfig.patch(patch);

	std::ofstream o("AutoRestartConfig.json");
	o << newConfig.dump(4);
}

std::string return_current_time_and_date()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	std::string unixtime = std::to_string(in_time_t);

	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-");
	ss << unixtime;
	return ss.str();
}