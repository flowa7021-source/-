#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace docvision {

// Status bar — bottom bar showing page info, zoom, view mode
class StatusBar {
public:
    StatusBar();
    ~StatusBar();

    bool initialize(HWND parentHwnd);

    // Update displayed info
    void setPageInfo(int currentPage, int totalPages);
    void setZoomLevel(double zoom);         // 0.1 to 5.0
    void setViewMode(const std::wstring& modeName);
    void setDocumentFormat(const std::wstring& format);
    void setStatusMessage(const std::wstring& message);
    void setProgressVisible(bool visible);
    void setProgress(double fraction);      // 0.0 to 1.0

    // Height
    int getHeight() const { return m_height; }

    // Callbacks
    using ZoomChangedCallback = std::function<void(double zoom)>;
    void onZoomChanged(ZoomChangedCallback cb) { m_onZoomChanged = cb; }

    using PageClickedCallback = std::function<void()>;
    void onPageClicked(PageClickedCallback cb) { m_onPageClicked = cb; }

    // Paint
    void paint(HDC hdc, const RECT& rect);

    // Mouse events
    void onMouseDown(int x, int y);
    void onMouseUp(int x, int y);

private:
    HWND m_parentHwnd = nullptr;
    int m_height = 24;

    int m_currentPage = 0;
    int m_totalPages = 0;
    double m_zoomLevel = 1.0;
    std::wstring m_viewMode;
    std::wstring m_format;
    std::wstring m_statusMessage;
    bool m_progressVisible = false;
    double m_progress = 0.0;

    ZoomChangedCallback m_onZoomChanged;
    PageClickedCallback m_onPageClicked;
};

} // namespace docvision
