#include "ui/pages_overview.h"
#include "core/document.h"
#include <algorithm>

namespace docvision {

PagesOverview::PagesOverview() {}
PagesOverview::~PagesOverview() {}

bool PagesOverview::initialize(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    return true;
}

void PagesOverview::setDocument(IDocument* document) {
    m_document = document;
    m_selectedPages.clear();
    m_thumbnails.clear();
}

void PagesOverview::show() { m_isVisible = true; renderThumbnails(); }
void PagesOverview::hide() { m_isVisible = false; }

void PagesOverview::setColumns(int columns) { m_columns = std::max(1, columns); }

void PagesOverview::selectPage(int pageIndex) {
    m_selectedPages.clear();
    m_selectedPages.push_back(pageIndex);
    if (m_onSelectionChanged) m_onSelectionChanged(m_selectedPages);
}

void PagesOverview::selectPages(const std::vector<int>& pageIndices) {
    m_selectedPages = pageIndices;
    if (m_onSelectionChanged) m_onSelectionChanged(m_selectedPages);
}

void PagesOverview::selectAll() {
    if (!m_document) return;
    m_selectedPages.clear();
    for (int i = 0; i < m_document->getPageCount(); ++i) {
        m_selectedPages.push_back(i);
    }
    if (m_onSelectionChanged) m_onSelectionChanged(m_selectedPages);
}

void PagesOverview::clearSelection() {
    m_selectedPages.clear();
    if (m_onSelectionChanged) m_onSelectionChanged(m_selectedPages);
}

bool PagesOverview::isPageSelected(int pageIndex) const {
    return std::find(m_selectedPages.begin(), m_selectedPages.end(), pageIndex) != m_selectedPages.end();
}

void PagesOverview::paint(HDC hdc, const RECT& rect) {
    if (!m_isVisible || !m_document) return;

    HBRUSH bg = CreateSolidBrush(RGB(220, 220, 220));
    FillRect(hdc, &rect, bg);
    DeleteObject(bg);

    int pageCount = m_document->getPageCount();
    int cellW = (rect.right - rect.left - 40) / m_columns;
    int cellH = static_cast<int>(cellW * 1.4);
    int padding = 10;

    HFONT font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < pageCount; ++i) {
        int col = i % m_columns;
        int row = i / m_columns;
        int x = rect.left + 20 + col * (cellW + padding);
        int y = rect.top + 20 + row * (cellH + padding + 20) - m_scrollOffset;

        if (y + cellH < rect.top || y > rect.bottom) continue;

        RECT cellRect = {x, y, x + cellW, y + cellH};

        // Selection highlight
        if (isPageSelected(i)) {
            HBRUSH selBrush = CreateSolidBrush(RGB(0, 120, 215));
            RECT selRect = {x - 3, y - 3, x + cellW + 3, y + cellH + 3};
            FillRect(hdc, &selRect, selBrush);
            DeleteObject(selBrush);
        }

        // Page background (white)
        HBRUSH pageBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &cellRect, pageBrush);
        DeleteObject(pageBrush);

        // Page border
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
        SelectObject(hdc, nullBr);
        Rectangle(hdc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        // Page number
        wchar_t num[16];
        swprintf_s(num, L"%d", i + 1);
        RECT numRect = {x, y + cellH + 2, x + cellW, y + cellH + 18};
        SetTextColor(hdc, isPageSelected(i) ? RGB(0, 120, 215) : RGB(80, 80, 80));
        DrawTextW(hdc, num, -1, &numRect, DT_CENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

int PagesOverview::hitTestPage(int x, int y) const {
    if (!m_document) return -1;
    // Simplified hit test
    int cellW = 200; // approximate
    int cellH = 280;
    int padding = 10;
    int col = (x - 20) / (cellW + padding);
    int row = (y + m_scrollOffset - 20) / (cellH + padding + 20);
    if (col < 0 || col >= m_columns) return -1;
    int index = row * m_columns + col;
    if (index < 0 || index >= m_document->getPageCount()) return -1;
    return index;
}

void PagesOverview::onMouseDown(int x, int y, bool ctrl, bool /*shift*/) {
    int page = hitTestPage(x, y);
    if (page < 0) return;

    if (ctrl) {
        // Toggle selection
        if (isPageSelected(page)) {
            m_selectedPages.erase(
                std::remove(m_selectedPages.begin(), m_selectedPages.end(), page),
                m_selectedPages.end());
        } else {
            m_selectedPages.push_back(page);
        }
    } else {
        m_selectedPages.clear();
        m_selectedPages.push_back(page);
    }

    if (m_onSelectionChanged) m_onSelectionChanged(m_selectedPages);
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void PagesOverview::onMouseDoubleClick(int x, int y) {
    int page = hitTestPage(x, y);
    if (page >= 0 && m_onDoubleClick) {
        m_onDoubleClick(page);
    }
}

void PagesOverview::onMouseWheel(short delta) {
    m_scrollOffset -= delta / 2;
    m_scrollOffset = std::max(0, m_scrollOffset);
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void PagesOverview::onKeyDown(UINT vkey) {
    if (vkey == VK_DELETE && m_document && m_document->canDeletePages()) {
        // Delete selected pages (in reverse order to preserve indices)
        auto sorted = m_selectedPages;
        std::sort(sorted.rbegin(), sorted.rend());
        for (int page : sorted) {
            m_document->deletePage(page);
        }
        m_selectedPages.clear();
        InvalidateRect(m_parentHwnd, nullptr, TRUE);
    }
    else if (vkey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        selectAll();
        InvalidateRect(m_parentHwnd, nullptr, TRUE);
    }
}

void PagesOverview::renderThumbnails() {
    // In production, this would render page thumbnails asynchronously
    m_thumbnails.clear();
}

} // namespace docvision
