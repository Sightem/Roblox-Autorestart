#include "Functions.h"

HANDLE Functions::GetHandleFromPID(DWORD pid)
{
	std::mutex m;
	std::unique_lock<std::mutex> lock(m);
	
	return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
}

void Functions::_usleep(int microseconds)
{
	std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

void Functions::_sleep(int miliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(miliseconds));
}

int Functions::CreateEulaAccept()
{
    HKEY hKey;
    DWORD dwValue = 1;
    DWORD dwType = REG_DWORD;
    LPCTSTR lpSubKey = TEXT("SOFTWARE\\Sysinternals\\Handle");
    LPCTSTR lpValueName = TEXT("EulaAccepted");

    if (RegCreateKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        if (RegSetValueEx(hKey, lpValueName, 0, dwType, (LPBYTE)&dwValue, sizeof(dwValue)) == ERROR_SUCCESS) {
            // Key was created and set successfully
            std::cout << "Key was created and set successfully" << std::endl;
            RegCloseKey(hKey);
        }
        else {
            // Failed to set value
            std::cout << "Failed to set value" << std::endl;
            RegCloseKey(hKey);
            return 1;
        }
    }
    else {
        std::cout << "Failed to create key" << std::endl;
        return 1;
    }
    return 0;
}

bool Functions::CheckEula()
{
	HKEY hKey;
	DWORD dwType = REG_DWORD;
	LPCTSTR lpSubKey = TEXT("SOFTWARE\\Sysinternals\\Handle");
	LPCTSTR lpValueName = TEXT("EulaAccepted");

	if (RegOpenKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		DWORD dwValue;
		DWORD dwSize = sizeof(dwValue);
		if (RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
			// Key was opened and value was read successfully
			std::cout << "Key was opened and value was read successfully" << std::endl;
			RegCloseKey(hKey);
			return true;
		}
		else {
			// Failed to read value
			std::cout << "Failed to read value" << std::endl;
			RegCloseKey(hKey);
			return false;
		}
	}
	else {
		std::cout << "Failed to open key" << std::endl;
		return false;
	}
}

std::vector<std::string> Functions::GetPaths(std::vector<DWORD> pids)
{
	FILE* fp;
	std::vector<std::string> paths;
	
	for (int i = 0; i < pids.size(); i++)
	{
		std::string command = "handle.exe -nobanner -p " + std::to_string(pids[i]);
		fp = _popen(command.c_str(), "r");
		if (fp == NULL) {
			std::cout << "Failed to run command" << std::endl;
			exit(1);
		}

		char buffer[MAX_PATH];
		std::string output;
		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
			if (std::string(buffer).find(".log") != std::string::npos)
			{
				output = buffer;
			}
		}

		_pclose(fp);

		std::regex re("File.+(\\w+:.+\\\\logs\\\\)([\\d+.]+_\\w+_Player_\\w+_last.log)");
		std::smatch match;
		std::regex_search(output, match, re);
		if (match.size() > 0)
		{
			paths.push_back(match[1].str() + match[2].str());
		}
	}

	return paths;
}