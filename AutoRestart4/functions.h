#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <regex>
#include <tchar.h>
#include <restartmanager.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>


#pragma comment(lib, "Rstrtmgr.lib")

namespace Functions
{
	DWORD GetProcessIDOfFileOpener(const std::string& path);

	bool TerminateProcessesByName(const std::string& processName);
	bool IsProcessRunning(const std::string& processName);
	bool checkProcessExists(DWORD pid, const std::string& processName);

	std::string readEntireFile(const std::string& filePath);
	std::string searchStringForPattern(const std::string& str, const std::string& pattern);
	std::string extractExeName(const std::string& path);
	std::string getRobloxPath();
	std::string getCurrentTimestamp();
	bool TerminateProcessTree(DWORD dwProcessId, UINT uExitCode);
	void flushConsole();
	void wait();

	std::vector<std::string> getLogFiles();

	std::string searchFileForPattern(const std::string& filePath, const std::string& pattern);
}