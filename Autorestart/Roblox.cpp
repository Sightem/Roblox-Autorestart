#include "Roblox.h"

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

std::string GetSmallestJobID(long long PlaceID, std::string cookie)
{
	Request req("https://games.roblox.com/v1/games/" + std::to_string(PlaceID) + "/servers/0?sortOrder=1&excludeFullGames=true&limit=10");
	req.set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36");
	req.set_cookie(".ROBLOSECURITY", cookie);
	req.set_header("Referer", "https://www.roblox.com/");
	req.set_header("Accept", "application/json");
	req.set_header("Content-Type", "application/json");
	req.initalize();

	Response res = req.get();

	json jres = json::parse(res.data);

	//if there are no jobs, throw an exception
	if (jres["data"].size() == 0) throw std::exception("No jobs found");

	//if maxplayer is 1 throw
	if (jres["data"][0]["maxPlayers"] == 1) throw std::exception("Max player count is 1");

	//if there is only one job, return it
	if (jres["data"].size() == 1) return jres["data"][0]["id"];

	std::map<long, std::string> jobs;
	for (int i = 0; i < jres["data"].size(); i++)
	{
		jobs[jres["data"][i]["ping"]] = jres["data"][i]["id"];
	}

	return jobs.begin()->second;
}