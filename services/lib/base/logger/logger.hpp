#pragma once

#include <atomic>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace logger {

enum class LogLevel : uint64_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

std::string levelToString(LogLevel level);

class Logger {
    class LogStream {
    public:
        LogStream(Logger& logger, LogLevel level)
            : logger_(logger), level_(level){};
        LogStream(const LogStream&) = delete;
        LogStream& operator=(const LogStream&) = delete;

        LogStream(LogStream&&) = default;
        LogStream& operator=(LogStream&&) = delete;

        ~LogStream();

        template <typename T>
        LogStream& operator<<(const T& value) {
            stream_ << value << ' ';
            return *this;
        }

    private:
        std::ostringstream stream_;
        LogLevel level_;
        Logger& logger_;
    };

public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    ~Logger();

    void SetLogFile(const std::string& file);
    void SetLogLevel(LogLevel level);
    LogStream Log(LogLevel level);
    void WriteLog(const std::string& data, LogLevel level);
    void NextPage();
    
    static Logger& Instance();
    
private:
    Logger() = default;
    
    void CloseFileStream();
    std::string GetCurrentTimestamp() const;

    LogLevel level_ = LogLevel::DEBUG;

    std::vector<std::string> CachedLogs_;
    std::mutex CacheMutex_;
    std::atomic<uint32_t> CurrentPage_;

    std::ofstream fileStream;
    std::mutex FileMutex_;
};

}  // namespace logger

#define LOG_DEBUG() ::logger::Logger::Instance().Log(logger::LogLevel::DEBUG)
#define LOG_INFO() ::logger::Logger::Instance().Log(logger::LogLevel::INFO)
#define LOG_WARN() ::logger::Logger::Instance().Log(logger::LogLevel::WARN)
#define LOG_ERROR() ::logger::Logger::Instance().Log(logger::LogLevel::ERROR)
#define LOG_CRITICAL() \
    ::logger::Logger::Instance().Log(logger::LogLevel::CRITICAL)
#define SET_LOG_FILE(filename) ::logger::Logger::Instance().SetLogFile(filename)
#define SET_LOG_LEVEL(level) ::logger::Logger::Instance().SetLogLevel(level)
#define LOG_NEXT_PAGE() ::logger::Logger::Instance().NextPage()
