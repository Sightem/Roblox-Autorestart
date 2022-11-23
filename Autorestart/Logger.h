#pragma once
#include <string>

enum LogLevel
{
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL
};

static std::string LogLevelNames[] =
{
	"DEBUG",
	"INFO",
	"WARNING",
	"ERROR",
	"FATAL"
};

// static log functions
void Log(const std::string_view text, LogLevel level = LOG_INFO, bool endl = 1);

void Log(const std::string_view, const std::string_view, bool endl = 1);