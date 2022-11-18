#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class Autorestart
{
public:
	void Start(bool);
	void UnlockRoblox();
	void KillRoblox();
	void _usleep(int microseconds);
	void _sleep(int miliseconds);
	
	bool ValidateCookies();
	bool FindRoblox();

private:
	std::vector<PROCESS_INFORMATION> robloxProcesses;
};
