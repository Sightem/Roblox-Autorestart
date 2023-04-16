#include "Autorestart.h"
#include "functions.h"
#include "json.hpp"

const nlohmann::json DEFAULT_CONFIG = nlohmann::json::parse(
	R"({
            "Timer": {
				"Enabled": true,
				"Time": 10
			},
            "PlaceID": 2414851778,
            "vip": {
                "Enabled": true,
                "url": ""
            },
            "SameServer": false,
            "ErrorPatterns": [],
            "WaitTimeAfterRestart": 10000,
            "RestartBrokenOnly": true
            }
        )");

int main()
{
	std::cout << "reading config...\n";
	if (!std::filesystem::exists("config.json"))
	{
		//logger.log("Config file not found, creating one...", WARNING, "AUTORESTART", true);
		std::cout << "Config file not found, creating one...\n";

		std::ofstream file("config.json");
		file << DEFAULT_CONFIG.dump(4);
		file.close();

		//logger.log("Config file created successfully! go ahead and edit it before running Autorestart again", WARNING, "AUTORESTART", true);
		std::cout << "Config file created successfully! go ahead and edit it before running Autorestart again\n";

		Functions::wait();
		return 1;
	}

	nlohmann::json config = nlohmann::json::parse(std::ifstream("config.json"));

	std::cout << "config read successfully\n";

	Functions::flushConsole();

	Autorestart autorestart(&config);
	autorestart.init();
	return 0;
}
