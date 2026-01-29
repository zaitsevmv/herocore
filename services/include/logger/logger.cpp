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

Logger::~Logger() {
    NextPage();
    CloseFileStream(); 
}

void Logger::WriteLog(const std::string& data, LogLevel level) {
    if (level < level_) return;
    std::string logData = "time=" + GetCurrentTimestamp() +
                           " level=" + levelToString(level) + " msg=" + data;
    std::lock_guard<std::mutex> lock(CacheMutex_);
    CachedLogs_.push_back(logData);
    // fileStream << logData << std::endl;
}

void Logger::NextPage() {
    std::lock_guard<std::mutex> fileLock(FileMutex_);
    std::lock_guard<std::mutex> cacheLock(CacheMutex_);
    for (const auto& log: CachedLogs_) {
        fileStream << std::format("page={0} {1}", CurrentPage_.load(std::memory_order_relaxed), log) << std::endl;
    }
    CachedLogs_.clear();
    CurrentPage_.fetch_add(1, std::memory_order::relaxed);
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
