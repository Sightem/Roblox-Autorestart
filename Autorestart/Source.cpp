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

void createcfg();
void CreateCookies();

int main(int argc, char* argv[])
{
	SetConsoleTitle("Roblox Autorestart");

	//-- Read launch arguments
	bool ontop = true;
	bool windowsize = true;
	bool forceminimize = false;
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

	//-- check if config.ini exists
	if (!fs::exists("config.ini"))
	{
		createcfg();
	}

	//-- check if cookies.txt exists
	if (!fs::exists("cookies.txt"))
	{
		CreateCookies();
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

void CreateCookies()
{
	std::ofstream file("cookies.txt");
	file << "";
	file.close();
}

void createcfg()
{
	std::ofstream config;
	config.open("config.ini");
	config << "placeid:";
	config << "\nvip:";
	config.close();
}