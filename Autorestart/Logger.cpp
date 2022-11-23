#include "Logger.h"
#include <iostream>

void Log(const std::string_view text, std::string_view function, bool endl)
{
	std::cout << '[' << function << ']' << ' ' << text << (endl ? '\n' : ' ');
}

void Log(const std::string_view text, LogLevel level, bool endl)
{
	std::cout << '[' << LogLevelNames[level] << ']' << ' ' << text << (endl ? '\n' : ' ');
}
