#define CURL_STATICLIB
#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <Lmcons.h>

//-- User libs
#include "Request.hpp"
#include "Autorestart.h"
#include "Roblox.h"
#include "Terminal.h"
#include "FolderSearch.h"
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

bool val_func(const fs::directory_entry& entry);
void createcfg();
std::vector<std::string> get_drives();


int main(int argc, char* argv[])
{
	SetConsoleTitle("Roblox Autorestart");

	//-- Read launch arguments
	bool ontop = 1;
	bool windowsize = 1;
	bool forceminimize = 0;
	if (argc > 1)
	{
		for (int i = 0; i < argc; i++)
		{
			if (strcmp(argv[i], "notop") == 0) { ontop = 0; }

			if (strcmp(argv[i], "nolockwindowsize") == 0) { windowsize = 0; }

			if (strcmp(argv[i], "minimize") == 0) { forceminimize = 1; }
		}
	}

	//-- Always on top
	if (ontop)
	{
		::SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		::ShowWindow(GetConsoleWindow(), SW_NORMAL);
	}

	//-- Window size lock
	if (windowsize)
	{
		SetWindowLong(GetConsoleWindow(), GWL_STYLE, GetWindowLong(GetConsoleWindow(), GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	}

	//-- Get current username 
	char username[UNLEN + 1];
	DWORD username_len = UNLEN + 1;
	GetUserName(username, &username_len);
	std::string username_str = username;


	//check if config.ini exists
	if (!fs::exists("config.ini"))
	{
		createcfg();
	}
	
	Autorestart autorestart;

	if (autorestart.validateCookie())
	{
		autorestart.start(forceminimize);
	}
	else
	{
		Log("Cookie validation failed.", LOG_ERROR);
		wait();
		return 0;
	}
}

bool val_func(const fs::directory_entry& entry)
{
	//-- check for a valid workspace folder
	if (entry.path().filename() == "workspace")
	{
		for (auto& file : fs::directory_iterator(entry.path().parent_path()))
		{
			if (file.is_directory())
			{
				if (file.path().filename() == "autoexec")
				{
					return true;
				}
			}
		}
	}
	return false;
}

void createcfg()
{
	//-- create config.ini
	
	std::ofstream config;
	config.open("config.ini");
	config << "placeid:";
	config << "\nvip:";
	config.close();
}

std::vector<std::string> get_drives()
{
	std::vector<std::string> drives;
	char drive_buffer[100];
	DWORD result = GetLogicalDriveStringsA(sizeof(drive_buffer), drive_buffer);
	if (result != 0)
	{
		char* drive_letter = drive_buffer;
		while (*drive_letter)
		{
			drives.push_back(drive_letter);
			drive_letter += strlen(drive_letter) + 1;
		}
	}
	return drives;
}