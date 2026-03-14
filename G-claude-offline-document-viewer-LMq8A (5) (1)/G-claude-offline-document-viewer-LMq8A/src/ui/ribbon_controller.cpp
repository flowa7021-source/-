#include "ui/ribbon_controller.h"
#include "diagnostics/logger.h"
#include <algorithm>

namespace docvision {

RibbonController::RibbonController() {
    buildTabDefinitions();
}

RibbonController::~RibbonController() {
    destroy();
}

bool RibbonController::initialize(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    createToolbar(parentHwnd);
    LOG_INFO("Ribbon controller initialized");
    return true;
}

void RibbonController::destroy() {
    if (m_toolbarHwnd) {
        DestroyWindow(m_toolbarHwnd);
        m_toolbarHwnd = nullptr;
    }
}

void RibbonController::buildTabDefinitions() {
    // Home tab — ALL tools concentrated here
    RibbonTabDef homeTab;
    homeTab.id = RibbonTab::Home;
    homeTab.name = L"Home";
    homeTab.groups = {
        {L"File", {CMD_FILE_OPEN, CMD_FILE_SAVE, CMD_FILE_SAVE_AS, CMD_FILE_CLOSE}},
        {L"View", {CMD_VIEW_SINGLE_PAGE, CMD_VIEW_CONTINUOUS, CMD_VIEW_TWO_PAGES,
                    CMD_VIEW_BOOK, CMD_VIEW_PAGES_OVERVIEW, CMD_VIEW_FULLSCREEN,
                    CMD_VIEW_SLIDESHOW}},
        {L"Zoom", {CMD_ZOOM_IN, CMD_ZOOM_OUT, CMD_ZOOM_FIT_WIDTH, CMD_ZOOM_FIT_PAGE,
                    CMD_ZOOM_FIT_HEIGHT, CMD_ZOOM_ACTUAL_SIZE}},
        {L"Navigate", {CMD_NAV_FIRST_PAGE, CMD_NAV_PREV_PAGE, CMD_NAV_NEXT_PAGE,
                        CMD_NAV_LAST_PAGE, CMD_NAV_GO_TO_PAGE, CMD_NAV_BACK, CMD_NAV_FORWARD}},
        {L"Search", {CMD_SEARCH_FIND, CMD_SEARCH_FIND_NEXT, CMD_SEARCH_FIND_PREV}},
        {L"OCR", {CMD_OCR_AREA, CMD_OCR_PAGE, CMD_OCR_DOCUMENT, CMD_OCR_MAKE_SEARCHABLE,
                   CMD_OCR_SET_LANGUAGE}},
        {L"Capture", {CMD_SCREENSHOT_AREA, CMD_COPY_SELECTION_IMAGE, CMD_COPY_TEXT}},
        {L"Annotate", {CMD_ANNOT_HIGHLIGHT, CMD_ANNOT_UNDERLINE, CMD_ANNOT_STRIKETHROUGH,
                        CMD_ANNOT_NOTE, CMD_ANNOT_INK, CMD_ANNOT_ERASER, CMD_ANNOT_TEXTBOX,
                        CMD_ANNOT_STAMP, CMD_ANNOT_HIDE_TOGGLE, CMD_ANNOT_LOCK,
                        CMD_ANNOT_PANEL_TOGGLE}},
        {L"Pages", {CMD_PAGE_DELETE, CMD_PAGE_ROTATE_LEFT, CMD_PAGE_ROTATE_RIGHT,
                     CMD_PAGE_CROP_AUTO, CMD_PAGE_CROP_MANUAL}},
        {L"Print", {CMD_PRINT}},
        {L"Display", {CMD_DISPLAY_BRIGHTNESS, CMD_DISPLAY_CONTRAST, CMD_DISPLAY_GAMMA,
                       CMD_DISPLAY_INVERT, CMD_DISPLAY_NIGHT_MODE, CMD_DISPLAY_RESET}},
        {L"Tools", {CMD_TOOL_HAND, CMD_TOOL_SELECT_TEXT, CMD_TOOL_SELECT_RECT}},
    };
    m_tabs.push_back(homeTab);

    // Annotate tab
    RibbonTabDef annotTab;
    annotTab.id = RibbonTab::Annotate;
    annotTab.name = L"Annotate";
    annotTab.groups = {
        {L"Markup", {CMD_ANNOT_HIGHLIGHT, CMD_ANNOT_UNDERLINE, CMD_ANNOT_STRIKETHROUGH}},
        {L"Draw", {CMD_ANNOT_INK, CMD_ANNOT_ERASER, CMD_ANNOT_SHAPE_RECT,
                    CMD_ANNOT_SHAPE_CIRCLE, CMD_ANNOT_SHAPE_LINE, CMD_ANNOT_SHAPE_ARROW}},
        {L"Text", {CMD_ANNOT_NOTE, CMD_ANNOT_TEXTBOX, CMD_ANNOT_STAMP}},
        {L"Manage", {CMD_ANNOT_HIDE_TOGGLE, CMD_ANNOT_LOCK, CMD_ANNOT_FLATTEN,
                      CMD_ANNOT_DELETE, CMD_ANNOT_PANEL_TOGGLE}},
    };
    m_tabs.push_back(annotTab);

    // Pages tab
    RibbonTabDef pagesTab;
    pagesTab.id = RibbonTab::Pages;
    pagesTab.name = L"Pages";
    pagesTab.groups = {
        {L"Organize", {CMD_PAGE_DELETE, CMD_PAGE_ROTATE_LEFT, CMD_PAGE_ROTATE_RIGHT,
                        CMD_PAGE_ROTATE_180}},
        {L"Crop", {CMD_PAGE_CROP_AUTO, CMD_PAGE_CROP_MANUAL}},
        {L"Export", {CMD_EXPORT_PAGE_PNG, CMD_EXPORT_PAGE_JPEG}},
    };
    m_tabs.push_back(pagesTab);

    // Tools tab
    RibbonTabDef toolsTab;
    toolsTab.id = RibbonTab::Tools;
    toolsTab.name = L"Tools";
    toolsTab.groups = {
        {L"OCR", {CMD_OCR_AREA, CMD_OCR_PAGE, CMD_OCR_DOCUMENT, CMD_OCR_MAKE_SEARCHABLE}},
        {L"Capture", {CMD_SCREENSHOT_AREA, CMD_COPY_SELECTION_IMAGE}},
    };
    m_tabs.push_back(toolsTab);

    // Help tab
    RibbonTabDef helpTab;
    helpTab.id = RibbonTab::Help;
    helpTab.name = L"Help";
    helpTab.groups = {
        {L"Info", {CMD_APP_HOTKEYS, CMD_APP_ABOUT}},
    };
    m_tabs.push_back(helpTab);
}

void RibbonController::createToolbar(HWND parentHwnd) {
    // In full implementation, this would create the Windows Ribbon Framework
    // or a custom toolbar with tab switching.
    // For now, reserve the ribbon height for layout purposes.
    m_ribbonHeight = 120;
}

void RibbonController::setCommandEnabled(UINT commandId, bool enabled) {
    m_commandEnabled[commandId] = enabled;
}

void RibbonController::setCommandChecked(UINT commandId, bool checked) {
    m_commandChecked[commandId] = checked;
}

void RibbonController::setMinimized(bool minimized) {
    m_isMinimized = minimized;
    m_ribbonHeight = minimized ? 30 : 120;
}

void RibbonController::selectTab(RibbonTab tab) {
    m_selectedTab = tab;
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void RibbonController::handleCommand(UINT commandId) {
    if (m_commandHandler) {
        m_commandHandler(commandId);
    }
}

void RibbonController::onDPIChanged(UINT dpi) {
    m_ribbonHeight = MulDiv(m_isMinimized ? 30 : 120, dpi, 96);
}

void RibbonController::paint(HDC hdc, const RECT& rect) {
    // Paint ribbon background
    HBRUSH bgBrush = CreateSolidBrush(RGB(245, 245, 245));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    // Paint tab headers
    int tabX = 10;
    HFONT font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    for (const auto& tab : m_tabs) {
        SIZE textSize;
        GetTextExtentPoint32W(hdc, tab.name.c_str(), static_cast<int>(tab.name.length()), &textSize);

        RECT tabRect = {tabX, rect.top + 2, tabX + textSize.cx + 20, rect.top + 28};

        if (tab.id == m_selectedTab) {
            HBRUSH selBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &tabRect, selBrush);
            DeleteObject(selBrush);
            SetTextColor(hdc, RGB(0, 120, 215));
        } else {
            SetTextColor(hdc, RGB(80, 80, 80));
        }

        DrawTextW(hdc, tab.name.c_str(), -1, &tabRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        tabX = tabRect.right + 5;
    }

    // Draw separator line
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, rect.left, rect.top + 30, nullptr);
    LineTo(hdc, rect.right, rect.top + 30);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

} // namespace docvision
