#include "Logger.h"

void Logger::print(const std::vector<std::ostream*>& stream_list, LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::string log_message = formatLogMessage(level, message);

	for (auto stream : stream_list)
	{
		(*stream) << log_message;
	}
}

void Logger::print(LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mtx);
	std::string log_message = formatLogMessage(level, message);
	for (auto stream : streams)
	{
		(*stream) << log_message;
	}
}


void Logger::addStream(std::ostream* stream)
{
    std::lock_guard<std::mutex> lock(mtx);
    streams.push_back(stream);
}

void Logger::setFormatString(const std::string& format)
{
    std::lock_guard<std::mutex> lock(mtx);
    format_string = format;
}

Logger& Logger::operator=(const Logger& other)
{
    std::lock_guard<std::mutex> lock(mtx);
    streams = other.streams;
    format_string = other.format_string;
    prefix = other.prefix;
    return *this;
}