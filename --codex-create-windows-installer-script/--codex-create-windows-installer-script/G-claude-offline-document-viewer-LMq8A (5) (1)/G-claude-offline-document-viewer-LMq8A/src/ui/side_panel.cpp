#include "ui/side_panel.h"
#include "core/document.h"

namespace docvision {

SidePanel::SidePanel() {}
SidePanel::~SidePanel() {}

bool SidePanel::initialize(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    return true;
}

void SidePanel::setMode(SidePanelMode mode) {
    m_mode = mode;
    m_isVisible = (mode != SidePanelMode::Hidden);
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void SidePanel::toggleMode(SidePanelMode mode) {
    if (m_mode == mode) {
        hide();
    } else {
        setMode(mode);
    }
}

void SidePanel::hide() {
    setMode(SidePanelMode::Hidden);
}

void SidePanel::setWidth(int width) {
    m_width = width;
}

void SidePanel::setOutline(const std::vector<OutlineItem>& outline) {
    m_outline = outline;
}

void SidePanel::setSearchResults(const std::vector<SearchResult>& results) {
    m_searchResults = results;
}

void SidePanel::setCurrentPage(int pageIndex) {
    m_currentPage = pageIndex;
}

void SidePanel::setPageCount(int count) {
    m_pageCount = count;
}

void SidePanel::paint(HDC hdc, const RECT& rect) {
    if (!m_isVisible) return;

    HBRUSH bgBrush = CreateSolidBrush(RGB(250, 250, 250));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    // Draw separator line on the right
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, rect.right - 1, rect.top, nullptr);
    LineTo(hdc, rect.right - 1, rect.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    switch (m_mode) {
        case SidePanelMode::Thumbnails: paintThumbnails(hdc, rect); break;
        case SidePanelMode::Outline: paintOutline(hdc, rect); break;
        case SidePanelMode::Annotations: paintAnnotations(hdc, rect); break;
        case SidePanelMode::SearchResults: paintSearchResults(hdc, rect); break;
        default: break;
    }
}

void SidePanel::paintThumbnails(HDC hdc, const RECT& rect) {
    // Draw thumbnail placeholders
    HFONT font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 100, 100));

    int y = rect.top + 10 - m_scrollOffset;
    int thumbW = m_width - 30;
    int thumbH = static_cast<int>(thumbW * 1.4);

    for (int i = 0; i < m_pageCount && y < rect.bottom; ++i) {
        if (y + thumbH > rect.top) {
            RECT thumbRect = {rect.left + 15, y, rect.left + 15 + thumbW, y + thumbH};

            // Highlight current page
            HBRUSH frameBrush;
            if (i == m_currentPage) {
                frameBrush = CreateSolidBrush(RGB(0, 120, 215));
            } else {
                frameBrush = CreateSolidBrush(RGB(200, 200, 200));
            }
            FrameRect(hdc, &thumbRect, frameBrush);
            DeleteObject(frameBrush);

            // Page number
            wchar_t pageNum[32];
            swprintf_s(pageNum, L"%d", i + 1);
            RECT numRect = {thumbRect.left, thumbRect.bottom + 2,
                             thumbRect.right, thumbRect.bottom + 16};
            DrawTextW(hdc, pageNum, -1, &numRect, DT_CENTER | DT_SINGLELINE);
        }
        y += thumbH + 24;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void SidePanel::paintOutline(HDC hdc, const RECT& rect) {
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(40, 40, 40));

    int y = rect.top + 10 - m_scrollOffset;
    for (const auto& item : m_outline) {
        if (y > rect.bottom) break;
        if (y + 20 > rect.top) {
            RECT textRect = {rect.left + 10, y, rect.right - 10, y + 20};
            DrawTextW(hdc, item.title.c_str(), -1, &textRect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        y += 22;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void SidePanel::paintAnnotations(HDC /*hdc*/, const RECT& /*rect*/) {
    // TODO: render annotation list
}

void SidePanel::paintSearchResults(HDC hdc, const RECT& rect) {
    HFONT font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    int y = rect.top + 10 - m_scrollOffset;
    for (const auto& result : m_searchResults) {
        if (y > rect.bottom) break;
        if (y + 36 > rect.top) {
            // Page number
            wchar_t pageInfo[64];
            swprintf_s(pageInfo, L"Page %d", result.pageIndex + 1);
            SetTextColor(hdc, RGB(0, 120, 215));
            RECT pageRect = {rect.left + 10, y, rect.right - 10, y + 16};
            DrawTextW(hdc, pageInfo, -1, &pageRect, DT_LEFT | DT_SINGLELINE);

            // Context text
            SetTextColor(hdc, RGB(80, 80, 80));
            RECT textRect = {rect.left + 10, y + 16, rect.right - 10, y + 34};
            std::wstring ctx = result.contextBefore + result.matchText + result.contextAfter;
            DrawTextW(hdc, ctx.c_str(), -1, &textRect,
                      DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        y += 40;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void SidePanel::onMouseDown(int /*x*/, int y) {
    // Handle clicks on outline items, thumbnails, search results
    if (m_mode == SidePanelMode::Thumbnails && m_pageCount > 0) {
        int thumbH = static_cast<int>((m_width - 30) * 1.4);
        int index = (y + m_scrollOffset - 10) / (thumbH + 24);
        if (index >= 0 && index < m_pageCount && m_onPageSelected) {
            m_onPageSelected(index);
        }
    }
}

void SidePanel::onMouseUp(int /*x*/, int /*y*/) {}
void SidePanel::onMouseMove(int /*x*/, int /*y*/) {}

void SidePanel::onMouseWheel(short delta) {
    m_scrollOffset -= delta / 3;
    m_scrollOffset = std::max(0, m_scrollOffset);
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

} // namespace docvision
