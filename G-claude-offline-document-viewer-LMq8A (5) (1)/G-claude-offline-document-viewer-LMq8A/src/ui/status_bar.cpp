#include "ui/status_bar.h"
#include <cstdio>

namespace docvision {

StatusBar::StatusBar() {}
StatusBar::~StatusBar() {}

bool StatusBar::initialize(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    return true;
}

void StatusBar::setPageInfo(int currentPage, int totalPages) {
    m_currentPage = currentPage;
    m_totalPages = totalPages;
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void StatusBar::setZoomLevel(double zoom) {
    m_zoomLevel = zoom;
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void StatusBar::setViewMode(const std::wstring& modeName) { m_viewMode = modeName; }
void StatusBar::setDocumentFormat(const std::wstring& format) { m_format = format; }
void StatusBar::setStatusMessage(const std::wstring& message) { m_statusMessage = message; }
void StatusBar::setProgressVisible(bool visible) { m_progressVisible = visible; }
void StatusBar::setProgress(double fraction) { m_progress = fraction; }

void StatusBar::paint(HDC hdc, const RECT& rect) {
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(0, 122, 204));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    HFONT font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    // Page info (left)
    wchar_t pageText[64];
    if (m_totalPages > 0) {
        swprintf_s(pageText, L"  Page %d / %d", m_currentPage + 1, m_totalPages);
    } else {
        swprintf_s(pageText, L"  No document");
    }
    RECT leftRect = rect;
    leftRect.right = leftRect.left + 200;
    DrawTextW(hdc, pageText, -1, &leftRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Status message (center)
    if (!m_statusMessage.empty()) {
        RECT centerRect = rect;
        centerRect.left = (rect.right - rect.left) / 3;
        centerRect.right = centerRect.left * 2;
        DrawTextW(hdc, m_statusMessage.c_str(), -1, &centerRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Zoom level (right)
    wchar_t zoomText[32];
    swprintf_s(zoomText, L"%d%%  ", static_cast<int>(m_zoomLevel * 100 + 0.5));
    RECT rightRect = rect;
    rightRect.left = rect.right - 120;
    DrawTextW(hdc, zoomText, -1, &rightRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // Format indicator
    if (!m_format.empty()) {
        RECT fmtRect = rect;
        fmtRect.left = rect.right - 200;
        fmtRect.right = rect.right - 130;
        DrawTextW(hdc, m_format.c_str(), -1, &fmtRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Progress bar
    if (m_progressVisible) {
        RECT progRect = {rect.left + 220, rect.top + 4,
                          rect.left + 220 + 200, rect.bottom - 4};
        HBRUSH trackBrush = CreateSolidBrush(RGB(0, 90, 158));
        FillRect(hdc, &progRect, trackBrush);
        DeleteObject(trackBrush);

        RECT fillRect = progRect;
        fillRect.right = fillRect.left + static_cast<int>((fillRect.right - fillRect.left) * m_progress);
        HBRUSH fillBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &fillRect, fillBrush);
        DeleteObject(fillBrush);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void StatusBar::onMouseDown(int x, int /*y*/) {
    // Click on page area -> go to page dialog
    if (x < 200 && m_onPageClicked) {
        m_onPageClicked();
    }
}

void StatusBar::onMouseUp(int /*x*/, int /*y*/) {}

} // namespace docvision
