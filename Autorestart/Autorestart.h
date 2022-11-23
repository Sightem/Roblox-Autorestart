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

	int GetInstanceCount();
	HANDLE GetHandleFromPID(DWORD pid);

	bool ValidateCookies();
	bool FindRoblox();
	bool FindFile(const std::string_view Directory, const std::string_view FileName);

private:
	std::vector<PROCESS_INFORMATION> robloxProcesses;
};
