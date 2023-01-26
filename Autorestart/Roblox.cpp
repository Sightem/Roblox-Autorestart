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

std::string GetSmallestJobID(const long long PlaceID, const std::string cookie, const int cookiecount)
{
    Request req("https://games.roblox.com/v1/games/" + std::to_string(PlaceID) + "/servers/0?sortOrder=1&excludeFullGames=true&limit=25");
    req.set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36");
    req.set_cookie(".ROBLOSECURITY", cookie);
    req.set_header("Referer", "https://www.roblox.com/");
    req.set_header("Accept", "application/json");
    req.set_header("Content-Type", "application/json");
    req.initalize();

    json jres = json::parse(req.get().data);
    json data = jres["data"];

    size_t size = data.size();

    //if there are no jobs, throw an exception
    if (size == 0) throw std::logic_error("No jobs found");

    //if maxplayer is 1 throw
    if (data[0]["maxPlayers"] == 1) throw std::logic_error("Max player count is 1");

    //if there is only one job, return it
    if (size == 1) return data[0]["id"];

    std::vector<GameJob> jobs(size);

    for (size_t i = 0; i < size; i++)
    {
        GameJob job;
        json temp = data[i];

        job.JobID = temp["id"];
        job.maxPlayers = temp["maxPlayers"];

        if (temp.contains("playing")) job.currentPlayers = temp["playing"]; else job.currentPlayers = job.maxPlayers;
        if (temp.contains("ping")) job.ping = temp["ping"]; else job.ping = 999999;

        jobs[i] = job;
    }

    std::sort(jobs.begin(), jobs.end(), [](GameJob a, GameJob b) { return a.ping < b.ping; });

    //check if job has enough space
    for (auto& job : jobs)
    {
        if (job.maxPlayers - job.currentPlayers >= cookiecount) return job.JobID;
    }
    //if no job has enough space, throw an exception
    throw std::logic_error("No job has enough space");
}

VIPServer getVIPServer(std::string url, std::string cookie)
{
	VIPServer server;
    
    server.LinkCode = url.substr(url.find("=") + 1);

    Request csrf("https://auth.roblox.com/v1/authentication-ticket");
    csrf.set_cookie(".ROBLOSECURITY", cookie);
    csrf.set_header("Referer", "https://www.roblox.com/");
    csrf.initalize();
    Response res = csrf.post();

    std::string csrfToken = res.headers["x-csrf-token"];

    Request accesscode(url);
    accesscode.set_cookie(".ROBLOSECURITY", cookie);
    accesscode.set_header("x-csrf-token", csrfToken);
    accesscode.set_header("Referer", "https://www.roblox.com/");
    accesscode.initalize();
    Response res2 = accesscode.get();

    std::regex regex("joinPrivateGame\\(\\d+\\, '(\\w+\\-\\w+\\-\\w+\\-\\w+\\-\\w+)");
    std::smatch match;
    std::regex_search(res2.data, match, regex);
    server.AccessCode = match[1];

	return server;
}

std::string getRobloxPath()
{
    std::string path;

    char value[255];
    DWORD BufferSize = 8192;
    RegGetValue(HKEY_CLASSES_ROOT, "roblox-player\\shell\\open\\command", "", RRF_RT_ANY, NULL, (PVOID)&value, &BufferSize);
    path = value;
    path = path.substr(1, path.length() - 5);

	return path;
}