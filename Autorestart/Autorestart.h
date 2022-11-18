#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class Autorestart
{
public:
	void start(bool);
	void unlockRoblox();
	void killRoblox();
	void _usleep(int microseconds);
	void _sleep(int miliseconds);
	
	bool validateCookie();
	bool findRoblox();

private:
	std::vector<PROCESS_INFORMATION> robloxProcesses;
};
