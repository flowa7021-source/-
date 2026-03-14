#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace docvision {

class RibbonController;
class TabManager;
class SidePanel;
class StatusBar;
class HomeScreen;
class ViewportManager;
class CommandManager;
class HotkeyManager;
class ThemeManager;

// Main application window
class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(HINSTANCE hInstance, int nCmdShow);
    int runMessageLoop();

    // Window handle
    HWND getHwnd() const { return m_hwnd; }
    HINSTANCE getInstance() const { return m_hInstance; }

    // Components
    RibbonController* getRibbon() const { return m_ribbon.get(); }
    TabManager* getTabManager() const { return m_tabManager.get(); }
    SidePanel* getSidePanel() const { return m_sidePanel.get(); }
    StatusBar* getStatusBar() const { return m_statusBar.get(); }
    HomeScreen* getHomeScreen() const { return m_homeScreen.get(); }
    CommandManager* getCommandManager() const { return m_commandManager.get(); }
    HotkeyManager* getHotkeyManager() const { return m_hotkeyManager.get(); }
    ThemeManager* getThemeManager() const { return m_themeManager.get(); }

    // Document operations
    void openFile(const std::wstring& path);
    void openFileDialog();
    void closeCurrentTab();
    void closeAllTabs();

    // Drag & drop
    void enableDragDrop();

    // Full screen / slide show
    void enterFullScreen();
    void exitFullScreen();
    void startSlideShow();
    bool isFullScreen() const { return m_isFullScreen; }

    // DPI
    void handleDPIChange(UINT dpi, const RECT* newRect);
    UINT getCurrentDPI() const { return m_currentDPI; }

    // CLI arguments
    void processCommandLine(const std::wstring& cmdLine);

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void onSize(int width, int height);
    void onPaint();
    void onClose();
    void onDestroy();
    void onDropFiles(HDROP hDrop);
    void onCommand(WORD commandId);
    void onKeyDown(WPARAM vkey, LPARAM flags);
    void onMouseWheel(short delta, WORD keys, int x, int y);
    void onTimer(UINT_PTR timerId);

    void layoutChildren();
    void updateTitle();
    void restoreSession();
    void saveSession();

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    UINT m_currentDPI = 96;
    bool m_isFullScreen = false;
    RECT m_preFullScreenRect{};
    DWORD m_preFullScreenStyle = 0;

    std::unique_ptr<RibbonController> m_ribbon;
    std::unique_ptr<TabManager> m_tabManager;
    std::unique_ptr<SidePanel> m_sidePanel;
    std::unique_ptr<StatusBar> m_statusBar;
    std::unique_ptr<HomeScreen> m_homeScreen;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<HotkeyManager> m_hotkeyManager;
    std::unique_ptr<ThemeManager> m_themeManager;
};

} // namespace docvision
