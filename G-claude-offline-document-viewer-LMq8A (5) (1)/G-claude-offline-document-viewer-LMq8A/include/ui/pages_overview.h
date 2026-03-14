#pragma once

#include <windows.h>
#include <vector>
#include <functional>

namespace docvision {

class IDocument;
struct PageBitmap;

// Pages overview — grid view of all pages for management operations
class PagesOverview {
public:
    PagesOverview();
    ~PagesOverview();

    bool initialize(HWND parentHwnd);
    void setDocument(IDocument* document);

    // Display
    void show();
    void hide();
    bool isVisible() const { return m_isVisible; }

    // Grid layout
    void setColumns(int columns);
    int getColumns() const { return m_columns; }

    // Selection
    void selectPage(int pageIndex);
    void selectPages(const std::vector<int>& pageIndices);
    void selectAll();
    void clearSelection();
    std::vector<int> getSelectedPages() const { return m_selectedPages; }
    bool isPageSelected(int pageIndex) const;

    // Callbacks
    using PageDoubleClickCallback = std::function<void(int pageIndex)>;
    void onPageDoubleClick(PageDoubleClickCallback cb) { m_onDoubleClick = cb; }

    using SelectionChangedCallback = std::function<void(const std::vector<int>&)>;
    void onSelectionChanged(SelectionChangedCallback cb) { m_onSelectionChanged = cb; }

    // Paint
    void paint(HDC hdc, const RECT& rect);

    // Mouse / keyboard
    void onMouseDown(int x, int y, bool ctrl, bool shift);
    void onMouseDoubleClick(int x, int y);
    void onMouseWheel(short delta);
    void onKeyDown(UINT vkey);

private:
    int hitTestPage(int x, int y) const;
    void renderThumbnails();

    HWND m_parentHwnd = nullptr;
    IDocument* m_document = nullptr;
    bool m_isVisible = false;
    int m_columns = 4;
    int m_scrollOffset = 0;

    std::vector<int> m_selectedPages;
    std::vector<PageBitmap> m_thumbnails;

    PageDoubleClickCallback m_onDoubleClick;
    SelectionChangedCallback m_onSelectionChanged;
};

} // namespace docvision
