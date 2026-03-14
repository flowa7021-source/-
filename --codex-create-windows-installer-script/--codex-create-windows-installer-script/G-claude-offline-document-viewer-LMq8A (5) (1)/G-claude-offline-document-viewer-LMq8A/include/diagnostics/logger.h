#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>

namespace docvision {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

// Logger — file-based logging with levels
class Logger {
public:
    static Logger& instance();

    bool initialize(const std::wstring& logDir, LogLevel minLevel = LogLevel::Info);
    void shutdown();

    void setMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel getMinLevel() const { return m_minLevel; }

    void log(LogLevel level, const char* file, int line, const std::string& message);
    void log(LogLevel level, const char* file, int line, const std::wstring& message);

    std::wstring getLogFilePath() const { return m_logFilePath; }

private:
    Logger() = default;

    std::wstring m_logFilePath;
    std::ofstream m_logFile;
    std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Info;
};

// Convenience macros
#define LOG_DEBUG(msg)   docvision::Logger::instance().log(docvision::LogLevel::Debug,   __FILE__, __LINE__, msg)
#define LOG_INFO(msg)    docvision::Logger::instance().log(docvision::LogLevel::Info,    __FILE__, __LINE__, msg)
#define LOG_WARNING(msg) docvision::Logger::instance().log(docvision::LogLevel::Warning, __FILE__, __LINE__, msg)
#define LOG_ERROR(msg)   docvision::Logger::instance().log(docvision::LogLevel::Error,   __FILE__, __LINE__, msg)

} // namespace docvision
