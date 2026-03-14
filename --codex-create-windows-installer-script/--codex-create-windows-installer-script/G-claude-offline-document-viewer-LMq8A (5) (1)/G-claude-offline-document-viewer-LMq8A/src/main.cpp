#include "ui/main_window.h"
#include "core/config_manager.h"
#include "diagnostics/logger.h"
#include "diagnostics/crash_handler.h"
#include "formats/format_registry.h"
#include "utils/file_utils.h"

#include <windows.h>
#include <shellapi.h>
#include <string>

using namespace docvision;

namespace {

// Get the directory where the executable resides
std::wstring getAppDirectory() {
    return utils::getExeDirectory();
}

// Initialize application subsystems
bool initializeApp() {
    std::wstring appDir = getAppDirectory();

    // Initialize config
    std::wstring configDir = appDir + L"\\config";
    utils::createDirectories(configDir);
    if (!ConfigManager::instance().initialize(configDir)) {
        return false;
    }

    // Initialize logging
    std::wstring logDir = appDir + L"\\logs";
    utils::createDirectories(logDir);

    LogLevel logLevel = LogLevel::Info;
    std::string levelStr = ConfigManager::instance().get<std::string>("logging.level", "info");
    if (levelStr == "debug") logLevel = LogLevel::Debug;
    else if (levelStr == "warning") logLevel = LogLevel::Warning;
    else if (levelStr == "error") logLevel = LogLevel::Error;

    Logger::instance().initialize(logDir, logLevel);
    LOG_INFO("DocVision starting...");
    LOG_INFO("App directory: " + utils::wideToUtf8(appDir));

    // Initialize crash handler
    std::wstring crashDir = logDir + L"\\crash";
    utils::createDirectories(crashDir);
    CrashHandler::instance().initialize(crashDir);

    LOG_INFO("Subsystems initialized successfully");
    return true;
}

void shutdownApp() {
    LOG_INFO("DocVision shutting down...");
    ConfigManager::instance().saveSettings();
    CrashHandler::instance().shutdown();
    Logger::instance().shutdown();
}

} // anonymous namespace

// WinMain entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    LPWSTR lpCmdLine, int nCmdShow) {
    // Initialize COM for shell operations
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"DocVision Error", MB_ICONERROR);
        return 1;
    }

    // Initialize common controls
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES |
                 ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // Initialize application
    if (!initializeApp()) {
        MessageBoxW(nullptr, L"Failed to initialize application.\nCheck logs for details.",
                    L"DocVision Error", MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    int exitCode = 0;

    {
        // Create and show main window
        MainWindow mainWindow;
        if (!mainWindow.create(hInstance, nCmdShow)) {
            LOG_ERROR("Failed to create main window");
            MessageBoxW(nullptr, L"Failed to create main window.",
                        L"DocVision Error", MB_ICONERROR);
            exitCode = 1;
        } else {
            // Process command line arguments
            if (lpCmdLine && lpCmdLine[0] != L'\0') {
                mainWindow.processCommandLine(lpCmdLine);
            }

            // Run message loop
            exitCode = mainWindow.runMessageLoop();
        }
    }

    shutdownApp();
    CoUninitialize();
    return exitCode;
}
