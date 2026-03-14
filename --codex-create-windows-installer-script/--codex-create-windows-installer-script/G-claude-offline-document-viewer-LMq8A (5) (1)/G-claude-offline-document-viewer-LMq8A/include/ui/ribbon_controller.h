#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace docvision {

// Ribbon tab IDs
enum class RibbonTab {
    Home = 0,
    Annotate,
    Pages,
    Tools,
    Help
};

// Ribbon command IDs (Home tab concentrates ALL tools)
enum RibbonCommandId : UINT {
    // View modes (Home)
    CMD_VIEW_SINGLE_PAGE = 1000,
    CMD_VIEW_CONTINUOUS,
    CMD_VIEW_TWO_PAGES,
    CMD_VIEW_BOOK,
    CMD_VIEW_BOOKLET,
    CMD_VIEW_PAGES_OVERVIEW,
    CMD_VIEW_FULLSCREEN,
    CMD_VIEW_SLIDESHOW,

    // Zoom (Home)
    CMD_ZOOM_IN = 1100,
    CMD_ZOOM_OUT,
    CMD_ZOOM_FIT_WIDTH,
    CMD_ZOOM_FIT_PAGE,
    CMD_ZOOM_FIT_HEIGHT,
    CMD_ZOOM_ACTUAL_SIZE,
    CMD_ZOOM_CUSTOM,

    // Navigation (Home)
    CMD_NAV_FIRST_PAGE = 1200,
    CMD_NAV_PREV_PAGE,
    CMD_NAV_NEXT_PAGE,
    CMD_NAV_LAST_PAGE,
    CMD_NAV_GO_TO_PAGE,
    CMD_NAV_BACK,
    CMD_NAV_FORWARD,

    // Search (Home)
    CMD_SEARCH_FIND = 1300,
    CMD_SEARCH_FIND_NEXT,
    CMD_SEARCH_FIND_PREV,

    // OCR (Home)
    CMD_OCR_AREA = 1400,
    CMD_OCR_PAGE,
    CMD_OCR_DOCUMENT,
    CMD_OCR_MAKE_SEARCHABLE,
    CMD_OCR_SET_LANGUAGE,

    // Screenshot / Capture (Home)
    CMD_SCREENSHOT_AREA = 1500,
    CMD_COPY_SELECTION_IMAGE,
    CMD_COPY_TEXT,

    // Annotations (Home + Annotate tab)
    CMD_ANNOT_HIGHLIGHT = 1600,
    CMD_ANNOT_UNDERLINE,
    CMD_ANNOT_STRIKETHROUGH,
    CMD_ANNOT_NOTE,
    CMD_ANNOT_INK,
    CMD_ANNOT_ERASER,
    CMD_ANNOT_TEXTBOX,
    CMD_ANNOT_STAMP,
    CMD_ANNOT_SHAPE_RECT,
    CMD_ANNOT_SHAPE_CIRCLE,
    CMD_ANNOT_SHAPE_LINE,
    CMD_ANNOT_SHAPE_ARROW,
    CMD_ANNOT_HIDE_TOGGLE,
    CMD_ANNOT_LOCK,
    CMD_ANNOT_FLATTEN,
    CMD_ANNOT_DELETE,
    CMD_ANNOT_PANEL_TOGGLE,

    // Page operations (Home + Pages tab)
    CMD_PAGE_DELETE = 1700,
    CMD_PAGE_ROTATE_LEFT,
    CMD_PAGE_ROTATE_RIGHT,
    CMD_PAGE_ROTATE_180,
    CMD_PAGE_CROP_AUTO,
    CMD_PAGE_CROP_MANUAL,

    // Print / Export (Home)
    CMD_PRINT = 1800,
    CMD_EXPORT_PAGE_PNG,
    CMD_EXPORT_PAGE_JPEG,

    // File operations
    CMD_FILE_OPEN = 1900,
    CMD_FILE_SAVE,
    CMD_FILE_SAVE_AS,
    CMD_FILE_CLOSE,
    CMD_FILE_CLOSE_ALL,
    CMD_FILE_PROPERTIES,

    // Comfort / Display (Home)
    CMD_DISPLAY_BRIGHTNESS = 2000,
    CMD_DISPLAY_CONTRAST,
    CMD_DISPLAY_GAMMA,
    CMD_DISPLAY_INVERT,
    CMD_DISPLAY_GRAYSCALE,
    CMD_DISPLAY_RESET,
    CMD_DISPLAY_NIGHT_MODE,

    // Tools
    CMD_TOOL_HAND = 2100,
    CMD_TOOL_SELECT_TEXT,
    CMD_TOOL_SELECT_RECT,

    // Application
    CMD_APP_SETTINGS = 2200,
    CMD_APP_HOTKEYS,
    CMD_APP_COMMAND_PALETTE,
    CMD_APP_HOME_SCREEN,
    CMD_APP_ABOUT,
};

// Ribbon group
struct RibbonGroup {
    std::wstring name;
    std::vector<UINT> commandIds;
};

// Ribbon tab definition
struct RibbonTabDef {
    RibbonTab id;
    std::wstring name;
    std::vector<RibbonGroup> groups;
};

// Ribbon controller — manages the toolbar/ribbon UI
class RibbonController {
public:
    RibbonController();
    ~RibbonController();

    bool initialize(HWND parentHwnd);
    void destroy();

    // Update command state (enabled/disabled, checked)
    void setCommandEnabled(UINT commandId, bool enabled);
    void setCommandChecked(UINT commandId, bool checked);

    // Get ribbon height for layout
    int getRibbonHeight() const { return m_ribbonHeight; }

    // Minimize/expand ribbon
    void setMinimized(bool minimized);
    bool isMinimized() const { return m_isMinimized; }

    // Tab management
    void selectTab(RibbonTab tab);
    RibbonTab getSelectedTab() const { return m_selectedTab; }

    // Command handler
    using CommandHandler = std::function<void(UINT commandId)>;
    void setCommandHandler(CommandHandler handler) { m_commandHandler = handler; }

    // Handle WM_COMMAND from ribbon
    void handleCommand(UINT commandId);

    // DPI change
    void onDPIChanged(UINT dpi);

    // Paint custom ribbon (if not using Windows Ribbon Framework)
    void paint(HDC hdc, const RECT& rect);

private:
    void buildTabDefinitions();
    void createToolbar(HWND parentHwnd);

    HWND m_parentHwnd = nullptr;
    HWND m_toolbarHwnd = nullptr;
    int m_ribbonHeight = 120;  // default height in pixels
    bool m_isMinimized = false;
    RibbonTab m_selectedTab = RibbonTab::Home;

    std::vector<RibbonTabDef> m_tabs;
    std::unordered_map<UINT, bool> m_commandEnabled;
    std::unordered_map<UINT, bool> m_commandChecked;
    CommandHandler m_commandHandler;
};

} // namespace docvision
