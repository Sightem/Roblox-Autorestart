#include "Manager.h"
#include "request.hpp"
#include "functions.h"
#include "Autorestart.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <regex>
#include <future>
#include <tchar.h>
#include <shlwapi.h>

std::string Manager::job_id;
std::mutex Manager::shared_string_mutex;

std::string Manager::GetJobIDString()
{
    std::scoped_lock<std::mutex> lock(shared_string_mutex);
    std::string value = job_id;
    return value;
}

void Manager::setJobIDString(const std::string& value)
{
    std::scoped_lock<std::mutex> lock(shared_string_mutex);
    job_id = value;
}

std::string Manager::GetRobloxTicket()
{
    Request req("https://auth.roblox.com/v1/authentication-ticket");
    req.set_cookie(".ROBLOSECURITY", cookie);
    req.set_header("User-Agent", USER_AGENT);
    req.set_header("Referer", "https://www.roblox.com/");
    req.set_header("x-csrf-token", csrf);
    req.initalize();
    Response res = req.post();

    // parse the headers to get the auth ticket
    std::string ticket = res.headers["rbx-authentication-ticket"];
    return ticket;
}

std::string Manager::GetUsername()
{
    Request req("https://users.roblox.com/v1/users/authenticated");
    req.set_cookie(".ROBLOSECURITY", cookie);
    req.set_header("x-csrf-token", csrf);
    req.set_header("User-Agent", USER_AGENT);
    req.set_header("Referer", "https://www.roblox.com/");
    req.set_header("accept", "application/json");
    req.initalize();

    Response res = req.get();

    nlohmann::json response = nlohmann::json::parse(res.data, nullptr, false);

    //return response["name"];
    if (response.is_discarded())
    {
		return "";
	}
    else
    {
		return response["name"];
	}
}

DWORD Manager::GetRobloxPID()
{
    std::set<DWORD> pidList = GetRobloxInstances();

    if (pidList.size() == 0)
    {
        return 0;
	}
    
    std::vector<std::string> logFiles = Native::GetLogFiles();

    for (const auto& file : logFiles)
    {
        std::string username = FS::SearchFileForPattern(file, R"(UserName%22%3a%22(.*?)%)");
        if (username == cookie_username)
        {
            roblox_log_path = file;
            return Native::GetProcessIDOfFileOpener(file);
		}
	}
	return 0;
}

std::set<DWORD> Manager::GetRobloxInstances()
{
    std::set<DWORD> pidSet;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (strcmp(entry.szExeFile, "RobloxPlayerBeta.exe") == 0)
            {
                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                BOOL debugged = FALSE;
                CheckRemoteDebuggerPresent(hProcess, &debugged);
                if (debugged)
                {
                    TerminateProcess(hProcess, 9);
                }
                else
                {
                    pidSet.insert(entry.th32ProcessID);
                }
                CloseHandle(hProcess);
            }
        }
    }

    CloseHandle(snapshot);

    return pidSet;
}

void Manager::GetVIPServerInfo()
{
    const Autorestart* restart = static_cast<const Autorestart*>(autorestart_ptr);

    std::string url = restart->config->at("vip").at("url");

    this->link_code = url.substr(url.find("=") + 1);

    return;
}

std::string Manager::GetCSRF()
{
    Request req("https://auth.roblox.com/v1/authentication-ticket");
    req.set_cookie(".ROBLOSECURITY", cookie);
    req.set_header("User-Agent", USER_AGENT);
    req.set_header("Referer", "https://www.roblox.com/");
    req.initalize();
    Response res = req.post();
    return res.headers["x-csrf-token"];
}

std::string Manager::GetSmallestJobID()
{
    const Autorestart* restart = static_cast<const Autorestart*>(autorestart_ptr);

    Request req("https://games.roblox.com/v1/games/" + std::to_string(restart->place_id) + "/servers/0?sortOrder=1&excludeFullGames=true&limit=25");
    req.set_header("User-Agent", USER_AGENT);
    req.set_cookie(".ROBLOSECURITY", cookie);
    req.set_header("Referer", "https://www.roblox.com/");
    req.set_header("Accept", "application/json");
    req.set_header("Content-Type", "application/json");
    req.initalize();

    nlohmann::json jres = nlohmann::json::parse(req.get().data);
    nlohmann::json data = jres["data"];

    size_t size = data.size();

    //if there are no jobs, throw an exception
    if (size == 0) throw std::runtime_error("No jobs found");

    //if maxplayer is 1 throw
    if (data[0]["maxPlayers"] == 1) throw std::runtime_error("Max player count is 1");

    //if there is only one job, return it
    if (size == 1) return data[0]["id"];

    std::vector<GameJob> jobs(size);

    for (size_t i = 0; i < size; i++)
    {
        GameJob job;
        nlohmann::json temp = data[i];

        job.JobID = temp["id"];
        job.maxPlayers = temp["maxPlayers"];

        if (temp.contains("playing")) job.currentPlayers = temp["playing"]; else job.currentPlayers = job.maxPlayers;
        if (temp.contains("ping")) job.ping = temp["ping"]; else job.ping = 999999;

        jobs[i] = job;
    }

    std::sort(jobs.begin(), jobs.end(), [](const GameJob& a, const GameJob& b) { return a.ping < b.ping; });

    //check if job has enough space
    for (auto& job : jobs)
    {
        if (job.maxPlayers - job.currentPlayers >= restart->cookies.size()) return job.JobID;
    }
    //if no job has enough space, throw an exception
    throw std::logic_error("No job has enough space");
}


void Manager::Init()
{
    const Autorestart* restart = static_cast<const Autorestart*>(autorestart_ptr);

    this->csrf = GetCSRF();

    this->cookie_username = GetUsername();
    if (cookie_username == "")
    {
		logger->log("Failed to get username from cookie, sleeping for 5 minutes to attempt recover...", ERR, "MANAGER", true);
        std::this_thread::sleep_for(std::chrono::minutes(5));
		Init();
	}

    if (restart->launch_vip)
    {
        GetVIPServerInfo();
    }

    LaunchRoblox();
}

void Manager::LaunchRoblox()
{
    Autorestart* restart = (Autorestart*)autorestart_ptr;

    std::string authticket = GetRobloxTicket();

    std::string browserTrackerID = "167200211022";

    std::string cmd;
    if (restart->launch_sameserver && GetJobIDString().empty())
    {
        try
        {
            job_id = GetSmallestJobID();
        }
        catch (std::exception& e)
        {
            logger->log("Failed to get JobID, defaulting to normal" + std::string(e.what()), WARNING, "MANAGER", true);

            restart->launch_sameserver = false;
        }
    }

    std::string unixtime = std::to_string(std::time(nullptr));

    if (restart->launch_vip)
    {
        cmd = '"' + restart->roblox_exe_path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestPrivateGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + std::to_string(restart->place_id) + "%26linkCode%3D" + link_code + "+browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
    }
    else if (restart->launch_sameserver)
    {
        cmd = '"' + restart->roblox_exe_path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestGameJob%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + std::to_string(restart->place_id) + "%26gameId%3D" + job_id + "%26isPlayTogetherGame%3Dfalse" + "+browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
    }
    else
    {
        cmd = '"' + restart->roblox_exe_path + '"' + " roblox-player:1+launchmode:play+gameinfo:" + authticket + "+launchtime" + ':' + unixtime + "+placelauncherurl:" + "https%3A%2F%2Fassetgame.roblox.com%2Fgame%2FPlaceLauncher.ashx%3Frequest%3DRequestGame%26browserTrackerId%3D" + browserTrackerID + "%26placeId%3D" + std::to_string(restart->place_id) + "%26isPlayTogetherGame%3Dfalse+" + "browsertrackerid:" + browserTrackerID + "+robloxLocale:en_us+gameLocale:en_us+channel:";
    }

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (!CreateProcess(&(restart->roblox_exe_path[0]), &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        logger->log("CreateProcess() failed: " + std::to_string(GetLastError()), ERR, "MANAGER", true);
    }
    DWORD wait = WaitForSingleObject(pi.hProcess, 120000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (wait != WAIT_OBJECT_0)
    {
		logger->log("Roblox failed to launch within 2 minutes", ERR, "MANAGER", true);
        Native::TerminateProcessTree(pi.dwProcessId, 9);

        pid = 0;
        return;
	}

    logger->log("Launched instance with username " + cookie_username, INFO, "MANAGER", true);

    //try to get the pid 20 times
    int attempt_count = 0;
    for (int i = 0; i < 20; i++)
    {
		pid = GetRobloxPID();
        if (pid != 0)
        {
			break;
		}
        attempt_count++;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

    if (attempt_count != 0 && pid != 0)
    {
		logger->log("Got PID associated with " + cookie_username + " after " + std::to_string(attempt_count) + " attempts", INFO, "MANAGER", true);
	}

    return;
}

void Manager::HandleMessage(Message& message)
{
    const Autorestart* restart = static_cast<const Autorestart*>(autorestart_ptr);

    switch (message.type)
    {
        case MessageType::WAIT:
        {
            std::chrono::milliseconds duration = std::get<std::chrono::milliseconds>(message.data);
            std::this_thread::sleep_for(duration);
            break;
        }
        case MessageType::CHECK_ROBLOX:
        {
            bool roblox_running = Native::CheckProcessExists(pid, "RobloxPlayerBeta.exe");
            if (!roblox_running)
            {
                roblox_log_path = "";
            }

            message.output = roblox_running;
            break;
        }
        case MessageType::CHECK_ERROR:
        {
            std::scoped_lock<std::mutex> lock(regex_mutex);

            std::string fileContents = FS::ReadEntireFile(roblox_log_path);

            std::vector<std::future<bool>> futures;

            for (const auto& pattern : restart->patterns)
            {
                futures.push_back(std::async(std::launch::async, [&fileContents, &pattern]() {
                    return !FS::SearchStringForPattern(fileContents, pattern).empty();
                    }));
            }

            bool error_found = false;
            for (auto& future : futures)
            {
                if (future.get())
                {
                    error_found = true;
                    break;
                }
            }

            message.output = error_found;
            break;
        }
        case MessageType::CHECK_SYNAPSE:
        {
            HWND hWnd = FindWindow(NULL, "Synapse X - Crash Reporter");
            if (hWnd != NULL)
            {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                TerminateRoblox();
            }
        }
        case MessageType::UPDATE_CSRF:
        {
            this->csrf = GetCSRF();
            break;
        }
    }
}

bool Manager::TerminateRoblox()
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) 
    {
        // Failed to open the process
        return false;
    }

    // Terminate the process
    BOOL success = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return (success != 0);
}
