#include "Roblox.h"
#include "Request.hpp"

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