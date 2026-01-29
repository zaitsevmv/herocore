#include "logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace logger {

std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::SetLogFile(const std::string& file) {
    if (fileStream.is_open()) CloseFileStream();
    fileStream.open(file);
}

void Logger::SetLogLevel(LogLevel level) { level_ = level; }

void Logger::CloseFileStream() { fileStream.close(); }

Logger::LogStream Logger::Log(LogLevel level) {
    return LogStream(*this, level);
}

Logger::~Logger() { CloseFileStream(); }

void Logger::WriteLog(const std::string& data, LogLevel level) {
    if (level < level_) return;
    std::string log_data = "time=" + GetCurrentTimestamp() +
                           " level=" + levelToString(level) + " msg=" + data;
    std::lock_guard<std::mutex> lock(mutex_);
    fileStream << log_data << std::endl;
}

std::string Logger::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();

    std::ostringstream ss;
    ss << now;
    return ss.str();
}

Logger::LogStream::~LogStream() { 
    logger_.WriteLog(stream_.str(), level_); 
}

}  // namespace logger
