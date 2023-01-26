#pragma once
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <regex>

#include "Request.hpp"
#include "json.hpp"

using json = nlohmann::json;

struct GameJob
{
    std::string JobID;
    int ping;
    int maxPlayers;
    int currentPlayers;
};

struct VIPServer
{
    std::string LinkCode, AccessCode;
};

std::string getRobloxTicket(std::string cookie);
VIPServer getVIPServer(std::string url, std::string cookie);
std::string GetSmallestJobID(const long long PlaceID, const std::string cookie, const int cookiecount);
std::string getRobloxPath();
std::vector<std::string> GetUsernames(std::vector<std::string> cookies);