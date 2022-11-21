#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class Autorestart
{
public:
	void Start();
	void UnlockRoblox();
	void KillRoblox();
	void _usleep(int microseconds);
	void _sleep(int miliseconds);
	void RobloxProcessWatcher();

	std::vector<HANDLE> GetRobloxProcesses();
	std::vector<HANDLE> GetProcessesByImageName(const char* image_name, size_t limit, DWORD access);
	
	bool ValidateCookies();
	bool FindRoblox();

private:
	std::vector<PROCESS_INFORMATION> robloxProcesses;
};
