#include "ui/home_screen.h"
#include <algorithm>

namespace docvision {

HomeScreen::HomeScreen() {}
HomeScreen::~HomeScreen() {}

bool HomeScreen::initialize(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    return true;
}

void HomeScreen::show() {
    m_isVisible = true;
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void HomeScreen::hide() {
    m_isVisible = false;
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void HomeScreen::setRecentFiles(const std::vector<RecentFileEntry>& files) {
    m_recentFiles = files;
    m_filteredFiles = files;
}

void HomeScreen::setHotkeySheet(const std::vector<std::pair<std::wstring, std::wstring>>& hotkeys) {
    m_hotkeySheet = hotkeys;
}

void HomeScreen::setSearchFilter(const std::wstring& filter) {
    m_searchFilter = filter;
    m_filteredFiles.clear();
    for (const auto& f : m_recentFiles) {
        if (filter.empty() ||
            f.title.find(filter) != std::wstring::npos ||
            f.path.find(filter) != std::wstring::npos) {
            m_filteredFiles.push_back(f);
        }
    }
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void HomeScreen::paint(HDC hdc, const RECT& rect) {
    if (!m_isVisible) return;

    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(245, 247, 250));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    paintHeader(hdc, rect);
    paintQuickActions(hdc, rect);
    paintRecentFiles(hdc, rect);

    if (m_showHotkeys) {
        paintHotkeySheet(hdc, rect);
    }
}

void HomeScreen::paintHeader(HDC hdc, const RECT& rect) {
    HFONT titleFont = CreateFontW(-32, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(30, 30, 30));

    RECT titleRect = {rect.left + 60, rect.top + 40, rect.right - 60, rect.top + 90};
    DrawTextW(hdc, L"DocVision", -1, &titleRect, DT_LEFT | DT_SINGLELINE);

    // Subtitle
    HFONT subFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    SelectObject(hdc, subFont);
    SetTextColor(hdc, RGB(120, 120, 120));

    RECT subRect = {rect.left + 60, rect.top + 90, rect.right - 60, rect.top + 112};
    DrawTextW(hdc, L"Offline Document Viewer  |  PDF  DjVu  CBZ  EPUB", -1, &subRect,
              DT_LEFT | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    DeleteObject(subFont);
}

void HomeScreen::paintQuickActions(HDC hdc, const RECT& rect) {
    HFONT font = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    struct ActionButton {
        const wchar_t* label;
        int x, y, w, h;
    };

    int startY = rect.top + 140;
    int startX = rect.left + 60;
    int btnW = 160, btnH = 40, gap = 15;

    ActionButton buttons[] = {
        {L"Open File", startX, startY, btnW, btnH},
        {L"Import Folder", startX + btnW + gap, startY, btnW, btnH},
        {L"Hotkeys", startX + (btnW + gap) * 2, startY, btnW, btnH},
        {L"Settings", startX + (btnW + gap) * 3, startY, btnW, btnH},
    };

    for (const auto& btn : buttons) {
        RECT btnRect = {btn.x, btn.y, btn.x + btn.w, btn.y + btn.h};

        // Button background
        HBRUSH btnBrush = CreateSolidBrush(RGB(0, 120, 215));
        HBRUSH hoverBrush = CreateSolidBrush(RGB(0, 100, 195));
        FillRect(hdc, &btnRect, btnBrush);
        DeleteObject(btnBrush);
        DeleteObject(hoverBrush);

        // Round corners simulation with FrameRect
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextW(hdc, btn.label, -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void HomeScreen::paintRecentFiles(HDC hdc, const RECT& rect) {
    int startY = rect.top + 210;

    // Section header
    HFONT headerFont = CreateFontW(-16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, headerFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(30, 30, 30));

    RECT headerRect = {rect.left + 60, startY, rect.right - 60, startY + 28};
    DrawTextW(hdc, L"Recent Files", -1, &headerRect, DT_LEFT | DT_SINGLELINE);

    // File list
    HFONT fileFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    SelectObject(hdc, fileFont);

    int y = startY + 40 - m_scrollOffset;
    int itemH = 52;
    int cardW = rect.right - rect.left - 120;

    for (size_t i = 0; i < m_filteredFiles.size() && y < rect.bottom; ++i) {
        if (y + itemH < rect.top + 200) {
            y += itemH + 4;
            continue;
        }

        const auto& file = m_filteredFiles[i];

        // Card background
        RECT cardRect = {rect.left + 60, y, rect.left + 60 + cardW, y + itemH};
        bool isHovered = (static_cast<int>(i) == m_hoveredItemIndex);

        HBRUSH cardBrush = CreateSolidBrush(isHovered ? RGB(235, 240, 248) : RGB(255, 255, 255));
        FillRect(hdc, &cardRect, cardBrush);
        DeleteObject(cardBrush);

        // Border
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(228, 228, 228));
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        SelectObject(hdc, nullBrush);
        Rectangle(hdc, cardRect.left, cardRect.top, cardRect.right, cardRect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        // Format badge
        SetTextColor(hdc, RGB(0, 120, 215));
        RECT fmtRect = {cardRect.left + 12, cardRect.top + 8,
                          cardRect.left + 60, cardRect.top + 24};
        DrawTextW(hdc, file.format.c_str(), -1, &fmtRect, DT_LEFT | DT_SINGLELINE);

        // Title
        SetTextColor(hdc, RGB(30, 30, 30));
        RECT titleRect = {cardRect.left + 65, cardRect.top + 8,
                            cardRect.right - 80, cardRect.top + 26};
        DrawTextW(hdc, file.title.c_str(), -1, &titleRect,
                  DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

        // Path
        SetTextColor(hdc, RGB(140, 140, 140));
        RECT pathRect = {cardRect.left + 65, cardRect.top + 28,
                           cardRect.right - 80, cardRect.top + 44};
        DrawTextW(hdc, file.path.c_str(), -1, &pathRect,
                  DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_PATH_ELLIPSIS);

        // Pin indicator
        if (file.isPinned) {
            SetTextColor(hdc, RGB(0, 120, 215));
            RECT pinRect = {cardRect.right - 40, cardRect.top + 16,
                              cardRect.right - 12, cardRect.top + 36};
            DrawTextW(hdc, L"\u2605", -1, &pinRect, DT_CENTER | DT_SINGLELINE); // star
        }

        y += itemH + 4;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(headerFont);
    DeleteObject(fileFont);
}

void HomeScreen::paintHotkeySheet(HDC hdc, const RECT& rect) {
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI Semibold");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    int x = rect.right - 350;
    int y = rect.top + 140;

    // Header
    SetTextColor(hdc, RGB(30, 30, 30));
    RECT headerRect = {x, y, x + 330, y + 24};
    DrawTextW(hdc, L"Keyboard Shortcuts", -1, &headerRect, DT_LEFT | DT_SINGLELINE);
    y += 30;

    // Hotkey entries
    HFONT monoFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    for (const auto& [keys, desc] : m_hotkeySheet) {
        if (y > rect.bottom - 20) break;

        // Description
        SetTextColor(hdc, RGB(80, 80, 80));
        RECT descRect = {x, y, x + 200, y + 18};
        SelectObject(hdc, font);
        DrawTextW(hdc, desc.c_str(), -1, &descRect, DT_LEFT | DT_SINGLELINE);

        // Keys
        SetTextColor(hdc, RGB(0, 100, 180));
        RECT keysRect = {x + 200, y, x + 330, y + 18};
        SelectObject(hdc, monoFont);
        DrawTextW(hdc, keys.c_str(), -1, &keysRect, DT_RIGHT | DT_SINGLELINE);

        y += 20;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
    DeleteObject(monoFont);
}

void HomeScreen::paintSearchBar(HDC /*hdc*/, const RECT& /*rect*/) {
    // TODO: search bar for filtering recent files
}

void HomeScreen::onMouseDown(int x, int y) {
    // Check quick action buttons
    int startY = 140;
    int btnH = 40;
    if (y >= startY && y <= startY + btnH) {
        int startX = 60;
        int btnW = 160, gap = 15;
        int col = (x - startX) / (btnW + gap);
        int relX = x - startX - col * (btnW + gap);
        if (relX >= 0 && relX <= btnW && m_onAction) {
            switch (col) {
                case 0: m_onAction(QuickAction::OpenFile); break;
                case 1: m_onAction(QuickAction::ImportFolder); break;
                case 2: m_showHotkeys = !m_showHotkeys; InvalidateRect(m_parentHwnd, nullptr, TRUE); break;
                case 3: m_onAction(QuickAction::Settings); break;
            }
            return;
        }
    }

    // Check recent file clicks
    int listY = 250 - m_scrollOffset;
    int itemH = 56;
    if (y >= listY) {
        int index = (y - listY) / itemH;
        if (index >= 0 && index < static_cast<int>(m_filteredFiles.size())) {
            if (m_onFileSelected) {
                m_onFileSelected(m_filteredFiles[index].path);
            }
        }
    }
}

void HomeScreen::onMouseUp(int /*x*/, int /*y*/) {}

void HomeScreen::onMouseMove(int /*x*/, int y) {
    int listY = 250 - m_scrollOffset;
    int itemH = 56;
    int newHover = -1;
    if (y >= listY) {
        newHover = (y - listY) / itemH;
        if (newHover >= static_cast<int>(m_filteredFiles.size())) newHover = -1;
    }
    if (newHover != m_hoveredItemIndex) {
        m_hoveredItemIndex = newHover;
        InvalidateRect(m_parentHwnd, nullptr, TRUE);
    }
}

void HomeScreen::onMouseWheel(short delta) {
    m_scrollOffset -= delta / 3;
    m_scrollOffset = std::max(0, m_scrollOffset);
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

} // namespace docvision
