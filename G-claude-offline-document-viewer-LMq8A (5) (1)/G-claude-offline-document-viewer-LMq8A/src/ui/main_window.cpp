#include "ui/main_window.h"
#include "ui/ribbon_controller.h"
#include "ui/tab_manager.h"
#include "ui/side_panel.h"
#include "ui/status_bar.h"
#include "ui/home_screen.h"
#include "ui/command_manager.h"
#include "ui/hotkey_manager.h"
#include "ui/theme_manager.h"
#include "core/config_manager.h"
#include "core/document.h"
#include "library/session_manager.h"
#include "diagnostics/logger.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <windowsx.h>

namespace docvision {

static const wchar_t* WINDOW_CLASS = L"DocVisionMainWindow";
static const wchar_t* WINDOW_TITLE = L"DocVision";

MainWindow::MainWindow()
    : m_ribbon(std::make_unique<RibbonController>())
    , m_tabManager(std::make_unique<TabManager>())
    , m_sidePanel(std::make_unique<SidePanel>())
    , m_statusBar(std::make_unique<StatusBar>())
    , m_homeScreen(std::make_unique<HomeScreen>())
    , m_commandManager(std::make_unique<CommandManager>())
    , m_hotkeyManager(std::make_unique<HotkeyManager>())
    , m_themeManager(std::make_unique<ThemeManager>())
{
}

MainWindow::~MainWindow() {
    saveSession();
}

bool MainWindow::create(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    // Register window class
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = windowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = WINDOW_CLASS;
    // wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

    if (!RegisterClassExW(&wcex)) {
        LOG_ERROR("Failed to register window class");
        return false;
    }

    // Get initial DPI
    HDC hdc = GetDC(nullptr);
    m_currentDPI = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);

    // Scale default window size by DPI
    int scaledWidth = MulDiv(1280, m_currentDPI, 96);
    int scaledHeight = MulDiv(800, m_currentDPI, 96);

    // Create window
    m_hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        scaledWidth, scaledHeight,
        nullptr, nullptr,
        hInstance,
        this // pass this pointer for WM_CREATE
    );

    if (!m_hwnd) {
        LOG_ERROR("Failed to create main window");
        return false;
    }

    // Initialize components
    m_ribbon->initialize(m_hwnd);
    m_tabManager->initialize(m_hwnd);
    m_sidePanel->initialize(m_hwnd);
    m_statusBar->initialize(m_hwnd);
    m_homeScreen->initialize(m_hwnd);
    m_hotkeyManager->initialize();

    // Set up command routing
    m_ribbon->setCommandHandler([this](UINT cmdId) { onCommand(cmdId); });

    // Set up hotkey executor
    m_hotkeyManager->setCommandExecutor([this](const std::string& cmdId) {
        m_commandManager->executeCommand(cmdId);
    });

    // Set up tab callbacks
    m_tabManager->onTabChanged([this](int /*tabId*/) {
        updateTitle();
        if (m_tabManager->hasOpenTabs()) {
            m_homeScreen->hide();
        }
    });

    m_tabManager->onAllTabsClosed([this]() {
        m_homeScreen->show();
        updateTitle();
    });

    // Set up home screen callbacks
    m_homeScreen->onFileSelected([this](const std::wstring& path) {
        openFile(path);
    });

    m_homeScreen->onAction([this](HomeScreen::QuickAction action) {
        switch (action) {
            case HomeScreen::QuickAction::OpenFile:
                openFileDialog();
                break;
            case HomeScreen::QuickAction::Settings:
                // TODO: show settings dialog
                break;
            default:
                break;
        }
    });

    // Set up status bar callbacks
    m_statusBar->onZoomChanged([this](double zoom) {
        if (auto* tab = m_tabManager->getSelectedTab()) {
            tab->viewport->setZoom(zoom);
        }
    });

    // Enable drag & drop
    enableDragDrop();

    // Restore session
    restoreSession();

    // Populate home screen with recent files
    auto& config = ConfigManager::instance();
    std::vector<HomeScreen::RecentFileEntry> recentEntries;
    for (const auto& path : config.getRecentFiles()) {
        HomeScreen::RecentFileEntry entry;
        entry.path = path;
        entry.title = utils::getFileName(path);
        entry.format = utils::getFileExtension(path);
        entry.isPinned = config.isFilePinned(path);
        recentEntries.push_back(entry);
    }
    m_homeScreen->setRecentFiles(recentEntries);

    // Show window
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    LOG_INFO("Main window created successfully");
    return true;
}

int MainWindow::runMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handleMessage(msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            onSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_PAINT:
            onPaint();
            return 0;

        case WM_CLOSE:
            onClose();
            return 0;

        case WM_DESTROY:
            onDestroy();
            return 0;

        case WM_DROPFILES:
            onDropFiles(reinterpret_cast<HDROP>(wParam));
            return 0;

        case WM_COMMAND:
            onCommand(LOWORD(wParam));
            return 0;

        case WM_KEYDOWN:
            onKeyDown(wParam, lParam);
            return 0;

        case WM_KEYUP:
            m_hotkeyManager->onKeyUp(static_cast<UINT>(wParam));
            return 0;

        case WM_MOUSEWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            WORD keys = GET_KEYSTATE_WPARAM(wParam);
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            onMouseWheel(delta, keys, x, y);
            return 0;
        }

        case WM_DPICHANGED:
            handleDPIChange(HIWORD(wParam), reinterpret_cast<const RECT*>(lParam));
            return 0;

        case WM_TIMER:
            onTimer(static_cast<UINT_PTR>(wParam));
            return 0;
    }

    return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

void MainWindow::onSize(int width, int height) {
    layoutChildren();
}

void MainWindow::onPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);

    // Paint components
    if (m_homeScreen->isVisible()) {
        m_homeScreen->paint(hdc, clientRect);
    }

    // Ribbon paints itself
    // Tab bar paints itself
    // Side panel paints itself
    // Status bar paints itself

    EndPaint(m_hwnd, &ps);
}

void MainWindow::onClose() {
    saveSession();

    // Check for unsaved documents
    // TODO: prompt user for unsaved changes

    DestroyWindow(m_hwnd);
}

void MainWindow::onDestroy() {
    PostQuitMessage(0);
}

void MainWindow::onDropFiles(HDROP hDrop) {
    UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);

    for (UINT i = 0; i < fileCount; ++i) {
        wchar_t path[MAX_PATH];
        DragQueryFileW(hDrop, i, path, MAX_PATH);

        if (DocumentFactory::isSupported(path)) {
            openFile(path);
        }
    }

    DragFinish(hDrop);
}

void MainWindow::onCommand(WORD commandId) {
    switch (commandId) {
        case CMD_FILE_OPEN:
            openFileDialog();
            break;
        case CMD_FILE_CLOSE:
            closeCurrentTab();
            break;
        case CMD_FILE_CLOSE_ALL:
            closeAllTabs();
            break;

        case CMD_VIEW_FULLSCREEN:
            if (m_isFullScreen) exitFullScreen();
            else enterFullScreen();
            break;
        case CMD_VIEW_SLIDESHOW:
            startSlideShow();
            break;

        case CMD_APP_HOME_SCREEN:
            m_homeScreen->show();
            break;

        // View modes
        case CMD_VIEW_SINGLE_PAGE:
        case CMD_VIEW_CONTINUOUS:
        case CMD_VIEW_TWO_PAGES:
        case CMD_VIEW_BOOK:
        case CMD_VIEW_BOOKLET:
        case CMD_VIEW_PAGES_OVERVIEW:
            if (auto* tab = m_tabManager->getSelectedTab()) {
                ViewMode mode = ViewMode::SinglePage;
                switch (commandId) {
                    case CMD_VIEW_CONTINUOUS: mode = ViewMode::ContinuousScroll; break;
                    case CMD_VIEW_TWO_PAGES: mode = ViewMode::TwoPages; break;
                    case CMD_VIEW_BOOK: mode = ViewMode::Book; break;
                    case CMD_VIEW_BOOKLET: mode = ViewMode::Booklet; break;
                    case CMD_VIEW_PAGES_OVERVIEW: mode = ViewMode::PagesOverview; break;
                    default: break;
                }
                tab->viewport->setViewMode(mode);
            }
            break;

        // Zoom
        case CMD_ZOOM_IN:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->zoomIn();
            break;
        case CMD_ZOOM_OUT:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->zoomOut();
            break;
        case CMD_ZOOM_ACTUAL_SIZE:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->zoomToActualSize();
            break;

        // Navigation
        case CMD_NAV_NEXT_PAGE:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->nextPage();
            break;
        case CMD_NAV_PREV_PAGE:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->previousPage();
            break;
        case CMD_NAV_FIRST_PAGE:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->firstPage();
            break;
        case CMD_NAV_LAST_PAGE:
            if (auto* tab = m_tabManager->getSelectedTab()) tab->viewport->lastPage();
            break;

        default:
            break;
    }

    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void MainWindow::onKeyDown(WPARAM vkey, LPARAM /*flags*/) {
    m_hotkeyManager->onKeyDown(static_cast<UINT>(vkey));

    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    m_hotkeyManager->processKeyEvent(static_cast<UINT>(vkey), ctrl, shift, alt);
}

void MainWindow::onMouseWheel(short delta, WORD keys, int /*x*/, int /*y*/) {
    bool ctrl = (keys & MK_CONTROL) != 0;

    if (ctrl) {
        // Ctrl + wheel = zoom
        if (auto* tab = m_tabManager->getSelectedTab()) {
            if (delta > 0) tab->viewport->zoomIn();
            else tab->viewport->zoomOut();
            m_statusBar->setZoomLevel(tab->viewport->getZoom());
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    } else {
        // Scroll
        if (auto* tab = m_tabManager->getSelectedTab()) {
            double scrollAmount = -delta * 0.5; // pixels to scroll
            tab->viewport->scrollBy(0, scrollAmount);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }
}

void MainWindow::onTimer(UINT_PTR /*timerId*/) {
    // Handle periodic tasks
}

void MainWindow::openFile(const std::wstring& path) {
    LOG_INFO("Opening file: " + utils::wideToUtf8(path));

    // Check if already open
    int existingTab = m_tabManager->findTabByPath(path);
    if (existingTab >= 0) {
        m_tabManager->selectTab(existingTab);
        return;
    }

    // Open in new tab
    int tabId = m_tabManager->addTab(path);
    if (tabId >= 0) {
        m_tabManager->selectTab(tabId);
        m_homeScreen->hide();

        // Add to recent files
        ConfigManager::instance().addRecentFile(path);

        updateTitle();
    } else {
        LOG_ERROR("Failed to open file: " + utils::wideToUtf8(path));
        MessageBoxW(m_hwnd,
                     (L"Failed to open file:\n" + path).c_str(),
                     L"DocVision", MB_ICONERROR);
    }
}

void MainWindow::openFileDialog() {
    std::wstring filter = utils::getSupportedFileFilter();
    std::wstring path = utils::showOpenFileDialog(m_hwnd, filter);
    if (!path.empty()) {
        openFile(path);
    }
}

void MainWindow::closeCurrentTab() {
    if (auto* tab = m_tabManager->getSelectedTab()) {
        m_tabManager->closeTab(tab->id);
    }
}

void MainWindow::closeAllTabs() {
    m_tabManager->closeAllTabs();
}

void MainWindow::enableDragDrop() {
    DragAcceptFiles(m_hwnd, TRUE);
}

void MainWindow::enterFullScreen() {
    if (m_isFullScreen) return;

    // Save current window state
    m_preFullScreenStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    GetWindowRect(m_hwnd, &m_preFullScreenRect);

    // Remove title bar and borders
    SetWindowLong(m_hwnd, GWL_STYLE, m_preFullScreenStyle & ~(WS_CAPTION | WS_THICKFRAME));

    // Get monitor info for current monitor
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST), &mi);

    SetWindowPos(m_hwnd, HWND_TOP,
                 mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left,
                 mi.rcMonitor.bottom - mi.rcMonitor.top,
                 SWP_NOZORDER | SWP_FRAMECHANGED);

    m_isFullScreen = true;
}

void MainWindow::exitFullScreen() {
    if (!m_isFullScreen) return;

    SetWindowLong(m_hwnd, GWL_STYLE, m_preFullScreenStyle);
    SetWindowPos(m_hwnd, nullptr,
                 m_preFullScreenRect.left, m_preFullScreenRect.top,
                 m_preFullScreenRect.right - m_preFullScreenRect.left,
                 m_preFullScreenRect.bottom - m_preFullScreenRect.top,
                 SWP_NOZORDER | SWP_FRAMECHANGED);

    m_isFullScreen = false;
}

void MainWindow::startSlideShow() {
    enterFullScreen();
    if (auto* tab = m_tabManager->getSelectedTab()) {
        tab->viewport->setViewMode(ViewMode::SlideShow);
    }
}

void MainWindow::handleDPIChange(UINT dpi, const RECT* newRect) {
    m_currentDPI = dpi;

    if (newRect) {
        SetWindowPos(m_hwnd, nullptr,
                     newRect->left, newRect->top,
                     newRect->right - newRect->left,
                     newRect->bottom - newRect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Notify components
    m_ribbon->onDPIChanged(dpi);
    m_themeManager->onDPIChanged(dpi);

    LOG_INFO("DPI changed to " + std::to_string(dpi));
}

void MainWindow::processCommandLine(const std::wstring& cmdLine) {
    if (cmdLine.empty()) return;

    // Simple parsing: treat as file path(s)
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(cmdLine.c_str(), &argc);
    if (!argv) return;

    for (int i = 0; i < argc; ++i) {
        std::wstring arg = argv[i];

        // Check if it's a file path
        if (utils::fileExists(arg) && DocumentFactory::isSupported(arg)) {
            openFile(arg);
        }
        // TODO: handle --page=N, --search=query, etc.
    }

    LocalFree(argv);
}

void MainWindow::layoutChildren() {
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    int w = clientRect.right - clientRect.left;
    int h = clientRect.bottom - clientRect.top;

    int y = 0;

    // Ribbon
    int ribbonH = m_ribbon->getRibbonHeight();
    y += ribbonH;

    // Tab bar
    int tabH = m_tabManager->getTabBarHeight();
    y += tabH;

    // Status bar at bottom
    int statusH = m_statusBar->getHeight();
    int contentH = h - y - statusH;

    // Side panel on left
    int sidePanelW = m_sidePanel->getWidth();

    // Main content area = rest
    int contentX = sidePanelW;
    int contentW = w - sidePanelW;

    // In practice these would be used to position child HWNDs or
    // to determine paint regions. The actual painting happens in
    // onPaint() or via child window painting.
    (void)contentX;
    (void)contentW;
    (void)contentH;
}

void MainWindow::updateTitle() {
    std::wstring title = WINDOW_TITLE;

    if (auto* tab = m_tabManager->getSelectedTab()) {
        title = tab->title + L" - " + WINDOW_TITLE;
        if (tab->isModified) {
            title = L"* " + title;
        }
    }

    SetWindowTextW(m_hwnd, title.c_str());
}

void MainWindow::restoreSession() {
    if (!ConfigManager::instance().get<bool>("general.restoreSession", true)) {
        return;
    }

    SessionManager sessionMgr;
    std::wstring sessionPath = ConfigManager::instance().getConfigDir() + L"\\session.json";
    sessionMgr.initialize(sessionPath);

    if (sessionMgr.hasSession()) {
        auto session = sessionMgr.loadSession();
        for (const auto& tab : session.tabs) {
            if (utils::fileExists(tab.filePath)) {
                openFile(tab.filePath);
            }
        }
    }
}

void MainWindow::saveSession() {
    SessionManager sessionMgr;
    std::wstring sessionPath = ConfigManager::instance().getConfigDir() + L"\\session.json";
    sessionMgr.initialize(sessionPath);

    SessionState state;
    auto tabSession = m_tabManager->getSessionData();
    for (const auto& tab : tabSession.tabs) {
        SessionState::TabState ts;
        ts.filePath = tab.filePath;
        ts.currentPage = tab.currentPage;
        ts.scrollX = tab.scrollX;
        ts.scrollY = tab.scrollY;
        ts.zoom = tab.zoom;
        state.tabs.push_back(ts);
    }
    state.activeTabIndex = tabSession.activeTabIndex;

    RECT wr;
    GetWindowRect(m_hwnd, &wr);
    state.windowX = wr.left;
    state.windowY = wr.top;
    state.windowWidth = wr.right - wr.left;
    state.windowHeight = wr.bottom - wr.top;
    state.windowMaximized = IsZoomed(m_hwnd) != 0;

    sessionMgr.saveSession(state);
}

} // namespace docvision
