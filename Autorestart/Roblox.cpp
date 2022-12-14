#include <vector>
#include <string>

#include "Roblox.h"
#include "Request.hpp"
#include "json.hpp"

using json = nlohmann::json;

std::string getRobloxTicket(std::string cookie) {
    // first we need to get the csrf token

    Request req("https://auth.roblox.com/v1/authentication-ticket");
    req.set_cookie(".ROBLOSECURITY", cookie);
    req.set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36");
    req.set_header("Referer", "https://www.roblox.com/");
    req.initalize();
    Response res = req.post();// This will fail but it will gives us the csrf token

    // now we need to parse the headers to get the csrf token
    std::string csrfToken = res.headers["x-csrf-token"];

    // now we repeat the same process but this time we will get the auth ticket

    // modify the headers to include the csrf token
    req.set_header("x-csrf-token", csrfToken);

    // repeat the request
    res = req.post();

    // parse the headers to get the auth ticket
    std::string ticket = res.headers["rbx-authentication-ticket"];
    return ticket;
}

std::vector<std::string> GetUsernames(std::vector<std::string> cookies) {
	std::vector<std::string> usernames;
	Request req("https://users.roblox.com/v1/users/authenticated");
	req.set_header("Referer", "https://www.roblox.com/");
	req.set_header("Accept", "application/json");
	req.set_header("Content-Type", "application/json");
	req.initalize();

	for (int i = 0; i < cookies.size(); i++)
	{
		req.set_cookie(".ROBLOSECURITY", cookies[i]);
		Response res = req.get();

		usernames.push_back(json::parse(res.data)["name"].get<std::string>());
	}
	return usernames;
}