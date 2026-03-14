#pragma once

#include "core/document.h"
#include <functional>

namespace docvision {

// View modes
enum class ViewMode {
    SinglePage,
    ContinuousScroll,
    TwoPages,       // facing pages
    Book,           // book spread (cover + facing)
    Booklet,
    PagesOverview,  // grid overview
    SlideShow,
    FullScreen
};

// Fit modes
enum class FitMode {
    FitWidth,
    FitPage,
    FitHeight,
    FitSmallestSide,
    ActualSize,
    CustomZoom
};

// Zoom preset percentages
constexpr int ZOOM_PRESETS[] = {10, 25, 33, 50, 66, 75, 100, 125, 150, 200, 300, 400, 500};
constexpr int ZOOM_PRESET_COUNT = 13;
constexpr double ZOOM_MIN = 0.10;
constexpr double ZOOM_MAX = 5.00;

// Viewport manager — handles zoom, scroll position, view modes, DPI
class ViewportManager {
public:
    ViewportManager();

    // View mode
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }

    // Zoom
    void setZoom(double zoom);          // 0.10 to 5.00
    void setFitMode(FitMode mode);
    double getZoom() const { return m_zoom; }
    FitMode getFitMode() const { return m_fitMode; }
    void zoomIn();                       // next preset step
    void zoomOut();                      // previous preset step
    void zoomToActualSize();
    void zoomToFitWidth(double viewportWidth, double pageWidth);
    void zoomToFitPage(double viewportWidth, double viewportHeight,
                       double pageWidth, double pageHeight);
    void zoomToFitHeight(double viewportHeight, double pageHeight);

    // Scroll position (in document coordinates)
    void setScrollPosition(double x, double y);
    double getScrollX() const { return m_scrollX; }
    double getScrollY() const { return m_scrollY; }
    void scrollBy(double dx, double dy);

    // Current page
    void setCurrentPage(int page);
    int getCurrentPage() const { return m_currentPage; }
    void goToPage(int page);
    void nextPage();
    void previousPage();
    void firstPage();
    void lastPage();

    // DPI
    void setDPI(int dpi);
    int getDPI() const { return m_dpi; }
    double getEffectiveScale() const;  // zoom * (dpi / 96.0)

    // Viewport size (window client area)
    void setViewportSize(int width, int height);
    int getViewportWidth() const { return m_viewportWidth; }
    int getViewportHeight() const { return m_viewportHeight; }

    // Document
    void setPageCount(int count) { m_pageCount = count; }
    int getPageCount() const { return m_pageCount; }

    // Continuous scroll helpers
    int getFirstVisiblePage() const;
    int getLastVisiblePage() const;
    int getBufferPagesBefore() const { return 2; } // prefetch
    int getBufferPagesAfter() const { return 3; }

    // Pages overview grid
    void setGridColumns(int cols) { m_gridColumns = cols; }
    int getGridColumns() const { return m_gridColumns; }

    // Change callbacks
    using ChangeCallback = std::function<void()>;
    void onViewChanged(ChangeCallback cb) { m_onViewChanged = cb; }
    void onPageChanged(ChangeCallback cb) { m_onPageChanged = cb; }

private:
    void notifyViewChanged();
    void notifyPageChanged();
    void clampScroll();

    ViewMode m_viewMode = ViewMode::SinglePage;
    FitMode m_fitMode = FitMode::FitWidth;
    double m_zoom = 1.0;
    double m_scrollX = 0;
    double m_scrollY = 0;
    int m_currentPage = 0;
    int m_pageCount = 0;
    int m_dpi = 96;
    int m_viewportWidth = 800;
    int m_viewportHeight = 600;
    int m_gridColumns = 4;

    ChangeCallback m_onViewChanged;
    ChangeCallback m_onPageChanged;
};

} // namespace docvision
