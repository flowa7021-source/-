#pragma once

#include "core/document.h"
#include <vector>
#include <string>

namespace docvision {

// Selection type
enum class SelectionType {
    None,
    Text,           // text selection for copy
    Rectangle       // area selection for screenshot/OCR
};

// Selection manager — handles text and area selections
class SelectionManager {
public:
    SelectionManager() = default;

    // Text selection
    void startTextSelection(int pageIndex, double x, double y);
    void updateTextSelection(double x, double y);
    void endTextSelection();
    bool hasTextSelection() const { return m_selectionType == SelectionType::Text && !m_selectedBlocks.empty(); }
    std::wstring getSelectedText() const;
    std::vector<Rect> getSelectionRects() const;

    // Rectangle selection (for screenshot / OCR)
    void startRectSelection(int pageIndex, double x, double y);
    void updateRectSelection(double x, double y);
    void endRectSelection();
    bool hasRectSelection() const { return m_selectionType == SelectionType::Rectangle && !m_selectionRect.isEmpty(); }
    Rect getSelectionRect() const { return m_selectionRect; }
    int getSelectionPage() const { return m_selectionPage; }

    // Clear selection
    void clearSelection();

    // Selection type
    SelectionType getSelectionType() const { return m_selectionType; }

    // Set text blocks for the current page (from document)
    void setPageTextBlocks(int pageIndex, const std::vector<TextBlock>& blocks);

private:
    SelectionType m_selectionType = SelectionType::None;
    int m_selectionPage = -1;
    double m_startX = 0, m_startY = 0;
    double m_endX = 0, m_endY = 0;
    Rect m_selectionRect;

    std::vector<TextBlock> m_currentPageBlocks;
    std::vector<TextBlock> m_selectedBlocks;
};

} // namespace docvision
