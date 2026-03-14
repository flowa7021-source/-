#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace docvision {

struct OutlineItem;
struct SearchResult;

// Side panel modes
enum class SidePanelMode {
    Hidden,
    Thumbnails,
    Outline,        // TOC / bookmarks
    Annotations,    // annotation list
    SearchResults
};

// Side panel — left-side switchable panel
class SidePanel {
public:
    SidePanel();
    ~SidePanel();

    bool initialize(HWND parentHwnd);

    // Mode
    void setMode(SidePanelMode mode);
    SidePanelMode getMode() const { return m_mode; }
    void toggleMode(SidePanelMode mode);
    void hide();
    bool isVisible() const { return m_mode != SidePanelMode::Hidden; }

    // Width
    int getWidth() const { return m_isVisible ? m_width : 0; }
    void setWidth(int width);

    // Content updates
    void setOutline(const std::vector<OutlineItem>& outline);
    void setSearchResults(const std::vector<SearchResult>& results);
    void setCurrentPage(int pageIndex);
    void setPageCount(int count);

    // Callbacks
    using PageSelectedCallback = std::function<void(int pageIndex)>;
    void onPageSelected(PageSelectedCallback cb) { m_onPageSelected = cb; }

    using SearchResultSelectedCallback = std::function<void(const SearchResult&)>;
    void onSearchResultSelected(SearchResultSelectedCallback cb) { m_onSearchResultSelected = cb; }

    // Paint
    void paint(HDC hdc, const RECT& rect);

    // Mouse events
    void onMouseDown(int x, int y);
    void onMouseUp(int x, int y);
    void onMouseMove(int x, int y);
    void onMouseWheel(short delta);

private:
    void paintThumbnails(HDC hdc, const RECT& rect);
    void paintOutline(HDC hdc, const RECT& rect);
    void paintAnnotations(HDC hdc, const RECT& rect);
    void paintSearchResults(HDC hdc, const RECT& rect);

    HWND m_parentHwnd = nullptr;
    SidePanelMode m_mode = SidePanelMode::Hidden;
    bool m_isVisible = false;
    int m_width = 250;
    int m_scrollOffset = 0;

    int m_currentPage = 0;
    int m_pageCount = 0;
    std::vector<OutlineItem> m_outline;
    std::vector<SearchResult> m_searchResults;

    PageSelectedCallback m_onPageSelected;
    SearchResultSelectedCallback m_onSearchResultSelected;
};

} // namespace docvision
