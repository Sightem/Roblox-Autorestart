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


namespace FS
{
	std::string SearchFileForPattern(const std::string& filePath, const std::string& pattern);
	std::string SearchStringForPattern(const std::string& str, const std::string& pattern);
	std::string ReadEntireFile(const std::string& filePath);
	std::string ExtractExeName(const std::string& path);
	std::string GetCurrentTimestamp();
	std::string GetRobloxPath();
}

namespace Native
{
	DWORD GetProcessIDOfFileOpener(const std::string& path);

	bool CheckProcessExists(DWORD pid, const std::string& processName);
	bool TerminateProcessesByName(const std::string& processName);
	bool TerminateProcessTree(DWORD dwProcessId, UINT uExitCode);
	bool IsProcessRunning(const std::string& processName);

	std::vector<std::string> GetLogFiles();
}

namespace Terminal
{
	void FlushConsole();
	void wait();
}