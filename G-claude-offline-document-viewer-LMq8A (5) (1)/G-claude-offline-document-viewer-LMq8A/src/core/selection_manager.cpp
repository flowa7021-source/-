#include "core/selection_manager.h"
#include <algorithm>
#include <cmath>

namespace docvision {

void SelectionManager::startTextSelection(int pageIndex, double x, double y) {
    clearSelection();
    m_selectionType = SelectionType::Text;
    m_selectionPage = pageIndex;
    m_startX = x;
    m_startY = y;
    m_endX = x;
    m_endY = y;
}

void SelectionManager::updateTextSelection(double x, double y) {
    if (m_selectionType != SelectionType::Text) return;
    m_endX = x;
    m_endY = y;

    // Find text blocks within selection range
    m_selectedBlocks.clear();
    double minX = std::min(m_startX, m_endX);
    double maxX = std::max(m_startX, m_endX);
    double minY = std::min(m_startY, m_endY);
    double maxY = std::max(m_startY, m_endY);

    for (const auto& block : m_currentPageBlocks) {
        // Check if block intersects with selection rectangle
        if (block.bounds.x + block.bounds.width >= minX &&
            block.bounds.x <= maxX &&
            block.bounds.y + block.bounds.height >= minY &&
            block.bounds.y <= maxY) {
            m_selectedBlocks.push_back(block);
        }
    }
}

void SelectionManager::endTextSelection() {
    // Selection is finalized
}

std::wstring SelectionManager::getSelectedText() const {
    std::wstring result;
    for (const auto& block : m_selectedBlocks) {
        if (!result.empty()) result += L" ";
        result += block.text;
    }
    return result;
}

std::vector<Rect> SelectionManager::getSelectionRects() const {
    std::vector<Rect> rects;
    for (const auto& block : m_selectedBlocks) {
        rects.push_back(block.bounds);
    }
    return rects;
}

void SelectionManager::startRectSelection(int pageIndex, double x, double y) {
    clearSelection();
    m_selectionType = SelectionType::Rectangle;
    m_selectionPage = pageIndex;
    m_startX = x;
    m_startY = y;
    m_endX = x;
    m_endY = y;
    m_selectionRect = {x, y, 0, 0};
}

void SelectionManager::updateRectSelection(double x, double y) {
    if (m_selectionType != SelectionType::Rectangle) return;
    m_endX = x;
    m_endY = y;
    m_selectionRect.x = std::min(m_startX, m_endX);
    m_selectionRect.y = std::min(m_startY, m_endY);
    m_selectionRect.width = std::abs(m_endX - m_startX);
    m_selectionRect.height = std::abs(m_endY - m_startY);
}

void SelectionManager::endRectSelection() {
    // Selection is finalized
}

void SelectionManager::clearSelection() {
    m_selectionType = SelectionType::None;
    m_selectionPage = -1;
    m_startX = m_startY = m_endX = m_endY = 0;
    m_selectionRect = {};
    m_selectedBlocks.clear();
}

void SelectionManager::setPageTextBlocks(int pageIndex, const std::vector<TextBlock>& blocks) {
    m_currentPageBlocks = blocks;
}

} // namespace docvision
