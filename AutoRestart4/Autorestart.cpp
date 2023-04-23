#include "Autorestart.h"
#include "logger.hpp"
#include "request.hpp"
#include "functions.h"

#include <regex>
#include <algorithm>
#include <filesystem>

bool Autorestart::ReadCookies()
{
	logger.log("Reading cookies from file...", INFO, "AUTORESTART", true);

	std::ifstream file("cookies.txt");
	if (file.peek() == std::ifstream::traits_type::eof())
	{
		logger.log("Cookies file is empty!", ERR, "AUTORESTART", true);
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		cookies.push_back(line);
	}

	logger.log("Cookies read successfully!", INFO, "AUTORESTART", true);
	return true;
}

bool Autorestart::ValidateCookies()
{
	logger.log("Validating cookies...", INFO, "AUTORESTART", true);

	const std::regex pattern(R"(_\|WARNING:-DO-NOT-SHARE-THIS.--Sharing-this-will-allow-someone-to-log-in-as-you-and-to-steal-your-ROBUX-and-items.\|_)");
	bool allCookiesValid = true;
	for (const auto& cookie : cookies)
	{
		auto cookieIter = std::find(cookies.begin(), cookies.end(), cookie);
		long long index = std::distance(cookies.begin(), cookieIter);

		if (!std::regex_search(cookie, pattern) || cookie.find("\"") != std::string::npos)
		{
			std::string errorMsg = "A cookie in Cookies.txt is invalid";
			errorMsg += !std::regex_search(cookie, pattern) ? ", the first part of the cookie is either corrupted or missing." : ", it contains quotes.";

			logger.log(errorMsg + " Invalid cookie on line: " + std::to_string(index + 1), ERR, "AUTORESTART", true);
			allCookiesValid = false;
		}
	}

	if (!allCookiesValid)
	{
		logger.log("Cookies are invalid, please fix them and try again.", ERR, "AUTORESTART", true);
		Terminal::wait();
		return false;
	}

	Request request("https://auth.roblox.com/v1/authentication-ticket");
	request.initalize();

	for (auto& cookie : cookies)
	{
		request.set_cookie(".ROBLOSECURITY", cookie);
		request.set_header("Referer", "https://www.roblox.com/");
		Response response = request.post();
		std::string csrfToken = response.headers["x-csrf-token"];

		long long index = std::distance(cookies.begin(), std::find(cookies.begin(), cookies.end(), cookie));
		if (csrfToken.empty())
		{
			logger.log("A cookie in Cookies.txt is invalid, or may also be expired. Invalid cookie on line: " + std::to_string(index + 1), ERR, "AUTORESTART", true);
			allCookiesValid = false;
		}
	}

	if (allCookiesValid)
	{
		logger.log("Cookies validated successfully!", INFO, "AUTORESTART", true);
	}

	return allCookiesValid;
}

void Autorestart::CreateLogsDirectoryIfNeeded()
{
	if (!std::filesystem::exists("logs"))
	{
		std::error_code ec;
		if (!std::filesystem::create_directory("logs", ec))
		{
			std::cerr << "Error creating logs directory: " << ec.message() << std::endl;
		}
	}
}

std::string Autorestart::GenerateLogFilePath()
{
	const std::string fileName = "Autorestart-log";
	const std::string fileExtension = ".txt";
	std::string timestamp = FS::GetCurrentTimestamp();
	std::string path = "logs/" + fileName + "-" + timestamp + fileExtension;
	return path;
}

std::string Autorestart::EnsureLogfile()
{
	CreateLogsDirectoryIfNeeded();

	std::string path = GenerateLogFilePath();

	std::ofstream logfile(path, std::ios::out | std::ios::trunc);
	if (!logfile.is_open())
	{
		std::cerr << "Error creating logfile: " << path << std::endl;
		return "";
	}
	logfile.close();

	return path;
}

void Autorestart::UnlockRoblox()
{
	CreateMutex(NULL, TRUE, "ROBLOX_singletonMutex");
}

bool Autorestart::HandleProblematicManagers(const std::vector<size_t>& problematic_managers)
{
	if (problematic_managers.size() == managers.size() && timer_enabled)
	{
		logger.log("All instances were broken, restarting all", INFO, "SCHEDULER", true);
		timer.terminate();
		return true;
	}
	for (size_t i : problematic_managers)
	{
		Manager& manager = managers[i];
		if (restart_broken_only)
		{
			logger.log(std::to_string(problematic_managers.size()) + " instances were broken, restarting the instances", INFO, "SCHEDULER", true);
			manager.SetJobIDString("");
			manager.TerminateRoblox();
			manager.LaunchRoblox();
		}
	}

	return false;
}

std::vector<size_t> Autorestart::CheckManagersForProblems()
{
	std::vector<size_t> problematic_managers;
	Message message = this->message_queue.top();
	this->message_queue.pop();

	for (size_t i = 0; i < this->managers.size(); ++i)
	{
		Manager& manager = managers[i];
		manager.HandleMessage(message);
		if (message.type == MessageType::CHECK_ROBLOX && message.output.has_value() && !message.output.value())
		{
			problematic_managers.push_back(i);
		}
		else if (message.type == MessageType::CHECK_ERROR && message.output.has_value() && message.output.value())
		{
			manager.TerminateRoblox();
			problematic_managers.push_back(i);
		}
	}

	return problematic_managers;
}

void Autorestart::EnqueueMessages()
{
	this->message_queue.push(Message(MessageType::CHECK_ROBLOX, "", 1));

	if (!patterns_empty)
		this->message_queue.push(Message(MessageType::CHECK_ERROR, "", 1));

	this->message_queue.push(Message(MessageType::CHECK_SYNAPSE, "", 1));
}

void Autorestart::UpdateCSRF()
{
	logger.log("Updating CSRF", INFO, "SCHEDULER", true);
	Message message(MessageType::UPDATE_CSRF, "", 2);
	for (auto& manager : this->managers)
	{
		manager.HandleMessage(message);
	}
}

void Autorestart::Scheduler()
{
	last_csrf_update = std::chrono::steady_clock::now();

	while (!terminate_scheduler)
	{
		if (!this->message_queue.empty())
		{
			auto problematic_managers = CheckManagersForProblems();
			if (HandleProblematicManagers(problematic_managers))
			{
				break;
			}
		}
		else if (!terminate_scheduler)
		{
			EnqueueMessages();
		}

		if (std::chrono::steady_clock::now() - last_csrf_update >= csrf_update_interval)
		{
			UpdateCSRF();
			last_csrf_update = std::chrono::steady_clock::now();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	return;
}

bool Autorestart::CookiesExists()
{
	if (std::filesystem::exists("cookies.txt"))
	{
		return true;
	}
	else
	{
		std::ofstream cookies("cookies.txt");
		cookies << "";
		cookies.close();
		return false;
	}
}

void Autorestart::Init()
{
	//-- Initialize logger
	logger.setLogFile(EnsureLogfile());
	logger.log("Initializing...", INFO, "AUTORESTART", true);

	//-- Set config values
	{
		wait_time_after_restart = std::chrono::milliseconds(config->at("WaitTimeAfterRestart"));
		restart_broken_only = config->at("RestartBrokenOnly");
		timer_enabled = config->at("Timer").at("Enabled");
		place_id = config->at("PlaceID");
		launch_vip = config->at("vip").at("Enabled");
		launch_sameserver = config->at("SameServer");
		roblox_exe_path = FS::GetRobloxPath();
		patterns = config->at("ErrorPatterns");
		patterns_empty = patterns.empty();
		time = config->at("Timer").at("Time");

		if (launch_vip)
		{
			std::string url = config->at("vip").at("url");

			this->link_code = url.substr(url.find("=") + 1);
		}
	}

	//-- Read cookies
	if (CookiesExists() && ReadCookies())
	{
		logger.log("Read cookies successfully", INFO, "AUTORESTART", true);
		
		if (!ValidateCookies())
			return;
	}
	else
	{
		logger.log("Failed to read cookies", ERR, "AUTORESTART", true);

		while (!logger.isDonePrinting()) 
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		Terminal::wait();
		return;
	}

	//-- Check if roblox is running
	std::string exe_name = FS::ExtractExeName(FS::GetRobloxPath());
	if (Native::IsProcessRunning("RobloxPlayerBeta.exe") || Native::IsProcessRunning(exe_name))
	{
		logger.log("Roblox is running, closing it...", WARNING, "AUTORESTART", true);

		Native::TerminateProcessesByName("RobloxPlayerBeta.exe");
		Native::TerminateProcessesByName(exe_name);

		logger.log("Roblox closed successfully! waiting for 5 seconds for roblox to complete it's clean up...", WARNING, "AUTORESTART", true);
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	//-- Initialize the managers
	logger.log("Initializing managers...", INFO, "AUTORESTART", true);


	while (true)
	{
		for (auto& cookie : cookies)
		{
			managers.emplace_back(cookie, &logger, this);
		}
		logger.log("Managers initialized successfully!", INFO, "AUTORESTART", true);

		//-- Start the managers
		logger.log("Starting managers...", INFO, "AUTORESTART", true);

		for (auto& manager : managers)
		{
			UnlockRoblox();
			manager.Init();
		}

		if (timer_enabled)
		{
			terminate_scheduler = false;
			scheduler_thread = std::thread(&Autorestart::Scheduler, this);

			timer.SetRestartTimer(time);
			timer.run();
		}
		else
		{
			logger.log("Timer is disabled, starting scheduler...", INFO, "AUTORESTART", true);
			Scheduler();
		}

		terminate_scheduler = true;

		//-- Drop all messages in the queue
		while (!message_queue.empty())
		{
			message_queue.pop();
		}
		
		//-- Terminate all managers
		for (auto& manager : managers)
		{
			manager.TerminateRoblox();
		}

		//-- Just in case
		Native::TerminateProcessesByName("RobloxPlayerBeta.exe");
		Native::TerminateProcessesByName(exe_name);

		//-- Clean up managers
		managers.clear();

		scheduler_thread.join();
		
		std::this_thread::sleep_for(wait_time_after_restart);
	}
}