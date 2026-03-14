#include "diagnostics/logger.h"
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <windows.h>

namespace docvision {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

bool Logger::initialize(const std::wstring& logDir, LogLevel minLevel) {
    m_minLevel = minLevel;

    // Create log directory
    std::filesystem::create_directories(logDir);

    // Generate log file name with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm;
    localtime_s(&tm, &time);

    std::wostringstream filename;
    filename << logDir << L"\\docvision_"
             << std::setfill(L'0')
             << std::setw(4) << (tm.tm_year + 1900)
             << std::setw(2) << (tm.tm_mon + 1)
             << std::setw(2) << tm.tm_mday
             << L".log";
    m_logFilePath = filename.str();

    // Open log file (append mode)
    std::string path8(m_logFilePath.begin(), m_logFilePath.end());
    m_logFile.open(path8, std::ios::app);
    if (!m_logFile.is_open()) {
        return false;
    }

    // Write header
    m_logFile << "\n=== DocVision Log Started at ";
    m_logFile << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    m_logFile << " ===\n" << std::flush;

    return true;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile << "=== Log Ended ===\n";
        m_logFile.close();
    }
}

static const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        default: return "?????";
    }
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& message) {
    if (level < m_minLevel) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_logFile.is_open()) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm;
    localtime_s(&tm, &time);

    // Extract just the filename from path
    const char* filename = file;
    const char* sep = strrchr(file, '\\');
    if (!sep) sep = strrchr(file, '/');
    if (sep) filename = sep + 1;

    m_logFile << std::put_time(&tm, "%H:%M:%S") << " "
              << "[" << levelToString(level) << "] "
              << filename << ":" << line << " - "
              << message << "\n" << std::flush;
}

void Logger::log(LogLevel level, const char* file, int line, const std::wstring& message) {
    // Convert wide string to UTF-8
    std::string msg8;
    int len = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len > 0) {
        msg8.resize(len - 1);
        WideCharToMultiByte(CP_UTF8, 0, message.c_str(), -1, msg8.data(), len, nullptr, nullptr);
    }
    log(level, file, line, msg8);
}

} // namespace docvision
