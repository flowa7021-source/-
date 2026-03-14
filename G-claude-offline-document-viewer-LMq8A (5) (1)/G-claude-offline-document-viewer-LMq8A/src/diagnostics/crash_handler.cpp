#include "diagnostics/crash_handler.h"
#include "diagnostics/logger.h"
#include <dbghelp.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

#pragma comment(lib, "dbghelp.lib")

namespace docvision {

CrashHandler* CrashHandler::s_instance = nullptr;

CrashHandler& CrashHandler::instance() {
    static CrashHandler inst;
    return inst;
}

bool CrashHandler::initialize(const std::wstring& crashDumpDir) {
    m_dumpDir = crashDumpDir;
    s_instance = this;

    std::filesystem::create_directories(crashDumpDir);

    // Set unhandled exception filter
    m_previousFilter = SetUnhandledExceptionFilter(unhandledExceptionFilter);

    LOG_INFO("Crash handler initialized, dump dir: " +
             std::string(crashDumpDir.begin(), crashDumpDir.end()));
    return true;
}

void CrashHandler::shutdown() {
    if (m_previousFilter) {
        SetUnhandledExceptionFilter(m_previousFilter);
        m_previousFilter = nullptr;
    }
    s_instance = nullptr;
}

bool CrashHandler::writeDump(const std::wstring& reason) {
    // Generate dump filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm;
    localtime_s(&tm, &time);

    std::wostringstream dumpPath;
    dumpPath << m_dumpDir << L"\\crash_"
             << std::setfill(L'0')
             << std::setw(4) << (tm.tm_year + 1900)
             << std::setw(2) << (tm.tm_mon + 1)
             << std::setw(2) << tm.tm_mday << L"_"
             << std::setw(2) << tm.tm_hour
             << std::setw(2) << tm.tm_min
             << std::setw(2) << tm.tm_sec
             << L"_" << reason << L".dmp";

    return writeMiniDump(nullptr, dumpPath.str());
}

LONG WINAPI CrashHandler::unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    if (!s_instance) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Generate dump filename
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm;
    localtime_s(&tm, &time);

    std::wostringstream dumpPath;
    dumpPath << s_instance->m_dumpDir << L"\\crash_"
             << std::setfill(L'0')
             << std::setw(4) << (tm.tm_year + 1900)
             << std::setw(2) << (tm.tm_mon + 1)
             << std::setw(2) << tm.tm_mday << L"_"
             << std::setw(2) << tm.tm_hour
             << std::setw(2) << tm.tm_min
             << std::setw(2) << tm.tm_sec
             << L".dmp";

    std::wstring path = dumpPath.str();
    s_instance->writeMiniDump(exceptionInfo, path);
    s_instance->showCrashDialog(path);

    return EXCEPTION_EXECUTE_HANDLER;
}

bool CrashHandler::writeMiniDump(EXCEPTION_POINTERS* exceptionInfo,
                                  const std::wstring& dumpPath) {
    HANDLE hFile = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0,
                               nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    MINIDUMP_EXCEPTION_INFORMATION* pMei = nullptr;

    if (exceptionInfo) {
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = exceptionInfo;
        mei.ClientPointers = FALSE;
        pMei = &mei;
    }

    BOOL result = MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData),
        pMei,
        nullptr,
        nullptr
    );

    CloseHandle(hFile);
    return result != FALSE;
}

void CrashHandler::showCrashDialog(const std::wstring& dumpPath) {
    std::wstring message =
        L"DocVision encountered an unexpected error and needs to close.\n\n"
        L"A crash dump has been saved to:\n" + dumpPath + L"\n\n"
        L"Please report this issue with the dump file attached.";

    MessageBoxW(nullptr, message.c_str(), L"DocVision - Crash Report",
                MB_OK | MB_ICONERROR);
}

} // namespace docvision
