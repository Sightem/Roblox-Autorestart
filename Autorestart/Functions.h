#pragma once

#include <Windows.h>
#include <thread>
#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <string>
#include <fstream>
#include <regex>
#include <filesystem>

namespace Functions
{
	void _usleep(int microseconds);
	void _sleep(int miliseconds);
	HANDLE GetHandleFromPID(DWORD pid);
	int CreateEulaAccept();
	bool CheckEula();
	void FileWatcher(const std::string& path, const std::chrono::seconds& interval, std::regex re, bool& terminate);
	std::vector<std::string> GetPaths(std::vector<DWORD> pids);
}