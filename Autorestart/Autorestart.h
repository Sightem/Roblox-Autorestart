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
	void WorkspaceWatcher();

	size_t GetRobloxProcesses();
	std::vector<HANDLE> GetProcessesByImageName(const char* image_name, size_t limit, DWORD access);
	
	bool ValidateCookies();
	bool FindRoblox();
	bool FindFile(const std::string_view Directory, const std::string_view FileName);

private:
	std::vector<PROCESS_INFORMATION> robloxProcesses;
};
