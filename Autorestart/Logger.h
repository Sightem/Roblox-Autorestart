#pragma once

#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <stdarg.h>

typedef std::chrono::system_clock Clock;
typedef Clock::time_point TimePoint;
typedef Clock::duration Duration;

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

static constexpr LogLevel LogLevelMinimum = LogLevel::Trace;

static const char* logLevelStrings[] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL"
};
class Logger
{
public:
    Logger(const std::vector<std::ostream*>& streams) : streams(streams) {}

    Logger(const Logger& other) : streams(other.streams), format_string(other.format_string) {}

    Logger() : streams({ &std::cout }) {}

    void print(const std::vector<std::ostream*>& stream_list, LogLevel level, const std::string& message);

    void print(LogLevel level, const std::string& message);

    void addStream(std::ostream* stream);
    
    void setFormatString(const std::string& format);

    void setPrefix(const std::string& prefix) { this->prefix = prefix ; }

    Logger& operator=(const Logger& other);


private:
    std::vector<std::ostream*> streams;
    std::string format_string;
	std::string prefix;
    std::mutex mtx;

    std::string formatLogMessage(LogLevel level, const std::string& message)
    {
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::stringstream ss;
        ss << std::put_time(&tm, format_string.c_str()) << " " << prefix << "[" << logLevelStrings[static_cast<int>(level)] << "] " << message << std::endl;
        return ss.str();
    }
};
