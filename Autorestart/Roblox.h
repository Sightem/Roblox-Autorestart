#pragma once
#include <vector>
#include <string>
#include <map>
#include <exception>

#include "Request.hpp"
#include "json.hpp"

using json = nlohmann::json;

std::string getRobloxTicket(std::string cookie);
std::string GetSmallestJobID(long long PlaceID, std::string cookie);
std::vector<std::string> GetUsernames(std::vector<std::string> cookies);