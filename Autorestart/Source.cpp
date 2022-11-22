#define CURL_STATICLIB
#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <Lmcons.h>
#include <filesystem>

//-- User libs
#include "Request.hpp"
#include "Autorestart.h"
#include "Roblox.h"
#include "Terminal.h"
#include "Logger.h"
#include "json.hpp"

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


void CreateConfig();
void CreateCookies();
void Compatibility();

int main(int argc, char* argv[])
{
	SetConsoleTitle("Roblox Autorestart");

	//-- Read launch arguments
	bool compat = false;
	if (argc > 1)
	{
		for (int i = 0; i < argc; i++)
		{
			if (strcmp(argv[i], "compat") == 0) { compat = 1; }
		}
	}

	//-- check if config.ini exists
	if (!fs::exists("AutoRestartConfig.json"))
	{
		std::cout << "Config file not found, creating" << std::endl;
		CreateConfig();
		std::cout << "Creation done please edit the config to your desire and re open the program" << std::endl;
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

	if (Autorestart().ValidateCookies())
	{
		Autorestart().Start();
	}
	else
	{
		Log("Cookie validation failed.", LOG_ERROR);
		wait();
		return 0;
	}
}

void Compatibility()
{
	if (std::filesystem::exists("AccountData.json"))
	{
		system("curl -LJOs https://github.com/Sightem/RAMDecrypt/releases/download/yes/Program.exe");

		json data;
		try
		{
			data = json::parse(std::ifstream("AccountData.json"));
		}
		catch (std::exception e)
		{
			system("Program.exe AccountData.json");
			data = json::parse(std::ifstream("AccountData.json"));
		}

		if (std::filesystem::exists("cookies.txt"))
		{
			for (int i = 0; i < data.size(); i++)
			{
				std::ofstream file("cookies.txt", std::ios::app);
				std::string str = data[i]["SecurityToken"];
				str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
				file << str << std::endl;
			}
		}
		else
		{
			std::ofstream file("cookies.txt");
			for (int i = 0; i < data.size(); i++)
			{
				std::string str = data[i]["SecurityToken"];
				str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
				file << str << std::endl;
			}
		}

	}
	else
	{
		std::cout << "AccountData.json not found, please place it in the same directory as this executable" << std::endl;
		system("pause");
	}
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
	
	ordered_json data =
	{
		{"Timer", 10},
		{"PlaceID", 1},
		{"TopMost", true},
		{"ForceMinimize", false},
		{"Resizablewindow", false},
		{"vip", {
			{"Enabled", false},
			{"url", ""}
			}
		},
		{"workspaceinteraction", {
			{"Enabled", false},
			{"Path", ""}
			}
		},
		{"Watchdog", true}
	};
	
	config << data.dump(4);
	
	config.close();
}