#include "core/viewport_manager.h"
#include <algorithm>
#include <cmath>

namespace docvision {

ViewportManager::ViewportManager() {}

void ViewportManager::setViewMode(ViewMode mode) {
    if (m_viewMode != mode) {
        m_viewMode = mode;
        notifyViewChanged();
    }
}

void ViewportManager::setZoom(double zoom) {
    zoom = std::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
    if (std::abs(m_zoom - zoom) > 0.001) {
        m_zoom = zoom;
        m_fitMode = FitMode::CustomZoom;
        notifyViewChanged();
    }
}

void ViewportManager::setFitMode(FitMode mode) {
    m_fitMode = mode;
    notifyViewChanged();
}

void ViewportManager::zoomIn() {
    // Find next preset above current zoom
    int currentPercent = static_cast<int>(m_zoom * 100 + 0.5);
    for (int preset : ZOOM_PRESETS) {
        if (preset > currentPercent) {
            setZoom(preset / 100.0);
            return;
        }
    }
    // Already at max preset, try 10% increment
    setZoom(std::min(m_zoom * 1.1, ZOOM_MAX));
}

void ViewportManager::zoomOut() {
    int currentPercent = static_cast<int>(m_zoom * 100 + 0.5);
    for (int i = ZOOM_PRESET_COUNT - 1; i >= 0; --i) {
        if (ZOOM_PRESETS[i] < currentPercent) {
            setZoom(ZOOM_PRESETS[i] / 100.0);
            return;
        }
    }
    setZoom(std::max(m_zoom * 0.9, ZOOM_MIN));
}

void ViewportManager::zoomToActualSize() {
    setZoom(1.0);
    m_fitMode = FitMode::ActualSize;
}

void ViewportManager::zoomToFitWidth(double viewportWidth, double pageWidth) {
    if (pageWidth > 0) {
        double zoom = viewportWidth / pageWidth;
        m_zoom = std::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
        m_fitMode = FitMode::FitWidth;
        notifyViewChanged();
    }
}

void ViewportManager::zoomToFitPage(double viewportWidth, double viewportHeight,
                                     double pageWidth, double pageHeight) {
    if (pageWidth > 0 && pageHeight > 0) {
        double zoomW = viewportWidth / pageWidth;
        double zoomH = viewportHeight / pageHeight;
        double zoom = std::min(zoomW, zoomH);
        m_zoom = std::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
        m_fitMode = FitMode::FitPage;
        notifyViewChanged();
    }
}

void ViewportManager::zoomToFitHeight(double viewportHeight, double pageHeight) {
    if (pageHeight > 0) {
        double zoom = viewportHeight / pageHeight;
        m_zoom = std::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
        m_fitMode = FitMode::FitHeight;
        notifyViewChanged();
    }
}

void ViewportManager::setScrollPosition(double x, double y) {
    m_scrollX = x;
    m_scrollY = y;
    clampScroll();
}

void ViewportManager::scrollBy(double dx, double dy) {
    m_scrollX += dx;
    m_scrollY += dy;
    clampScroll();
    notifyViewChanged();
}

void ViewportManager::setCurrentPage(int page) {
    page = std::clamp(page, 0, std::max(0, m_pageCount - 1));
    if (m_currentPage != page) {
        m_currentPage = page;
        notifyPageChanged();
    }
}

void ViewportManager::goToPage(int page) {
    setCurrentPage(page);
}

void ViewportManager::nextPage() {
    if (m_currentPage < m_pageCount - 1) {
        setCurrentPage(m_currentPage + 1);
    }
}

void ViewportManager::previousPage() {
    if (m_currentPage > 0) {
        setCurrentPage(m_currentPage - 1);
    }
}

void ViewportManager::firstPage() {
    setCurrentPage(0);
}

void ViewportManager::lastPage() {
    setCurrentPage(m_pageCount - 1);
}

void ViewportManager::setDPI(int dpi) {
    if (m_dpi != dpi) {
        m_dpi = dpi;
        notifyViewChanged();
    }
}

double ViewportManager::getEffectiveScale() const {
    return m_zoom * (static_cast<double>(m_dpi) / 96.0);
}

void ViewportManager::setViewportSize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    clampScroll();
}

int ViewportManager::getFirstVisiblePage() const {
    // Simplified: in continuous mode, estimate based on scroll position
    return m_currentPage;
}

int ViewportManager::getLastVisiblePage() const {
    // Simplified: estimate visible pages based on viewport
    int visible = std::max(1, m_viewportHeight / 800); // rough estimate
    return std::min(m_currentPage + visible, m_pageCount - 1);
}

void ViewportManager::notifyViewChanged() {
    if (m_onViewChanged) m_onViewChanged();
}

void ViewportManager::notifyPageChanged() {
    if (m_onPageChanged) m_onPageChanged();
}

void ViewportManager::clampScroll() {
    m_scrollX = std::max(0.0, m_scrollX);
    m_scrollY = std::max(0.0, m_scrollY);
}

} // namespace docvision
