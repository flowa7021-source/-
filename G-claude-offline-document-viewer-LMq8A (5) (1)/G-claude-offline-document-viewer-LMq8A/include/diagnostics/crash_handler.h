#pragma once

#include <string>
#include <windows.h>

namespace docvision {

// Crash handler — writes minidump on crash
class CrashHandler {
public:
    static CrashHandler& instance();

    // Initialize crash handling
    bool initialize(const std::wstring& crashDumpDir);
    void shutdown();

    // Get crash dump directory
    std::wstring getCrashDumpDir() const { return m_dumpDir; }

    // Manual dump (for testing or diagnostics)
    bool writeDump(const std::wstring& reason = L"ManualDump");

private:
    CrashHandler() = default;

    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    bool writeMiniDump(EXCEPTION_POINTERS* exceptionInfo, const std::wstring& dumpPath);
    void showCrashDialog(const std::wstring& dumpPath);

    std::wstring m_dumpDir;
    LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter = nullptr;
    static CrashHandler* s_instance;
};

} // namespace docvision
