#include "ui/command_palette.h"
#include "ui/command_manager.h"

namespace docvision {

CommandPalette::CommandPalette() {}
CommandPalette::~CommandPalette() {}

void CommandPalette::initialize(HWND parentHwnd, CommandManager* commandManager) {
    m_parentHwnd = parentHwnd;
    m_commandManager = commandManager;
}

void CommandPalette::show() {
    m_isVisible = true;
    m_query.clear();
    m_selectedIndex = 0;
    updateResults();
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void CommandPalette::hide() {
    m_isVisible = false;
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

bool CommandPalette::handleKeyDown(UINT vkey) {
    if (!m_isVisible) return false;

    switch (vkey) {
        case VK_ESCAPE: hide(); return true;
        case VK_RETURN: executeSelected(); hide(); return true;
        case VK_UP:
            if (m_selectedIndex > 0) --m_selectedIndex;
            InvalidateRect(m_parentHwnd, nullptr, TRUE);
            return true;
        case VK_DOWN:
            if (m_selectedIndex + 1 < static_cast<int>(m_results.size())) ++m_selectedIndex;
            InvalidateRect(m_parentHwnd, nullptr, TRUE);
            return true;
        case VK_BACK:
            if (!m_query.empty()) { m_query.pop_back(); updateResults(); }
            return true;
    }
    return false;
}

void CommandPalette::handleChar(wchar_t ch) {
    if (!m_isVisible || ch < 32) return;
    m_query += ch;
    m_selectedIndex = 0;
    updateResults();
}

void CommandPalette::updateResults() {
    m_results.clear();
    if (!m_commandManager) return;

    auto commands = m_query.empty()
        ? m_commandManager->getAllCommands()
        : m_commandManager->searchCommands(m_query);

    for (const auto* cmd : commands) {
        PaletteEntry entry;
        entry.commandId = cmd->id;
        entry.name = cmd->name;
        entry.description = cmd->description;
        m_results.push_back(entry);
    }
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

void CommandPalette::executeSelected() {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_results.size())) {
        m_commandManager->executeCommand(m_results[m_selectedIndex].commandId);
    }
}

void CommandPalette::paint(HDC hdc, const RECT& parentRect) {
    if (!m_isVisible) return;

    int paletteW = 500;
    int paletteH = 400;
    int x = (parentRect.right - parentRect.left - paletteW) / 2;
    int y = parentRect.top + 80;

    RECT bgRect = {x, y, x + paletteW, y + paletteH};

    // Shadow
    RECT shadowRect = {x + 3, y + 3, x + paletteW + 3, y + paletteH + 3};
    HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);

    // Background
    HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &bgRect, bg);
    DeleteObject(bg);

    // Border
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hdc, nullBrush);
    Rectangle(hdc, bgRect.left, bgRect.top, bgRect.right, bgRect.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    // Search input
    HFONT font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    RECT inputRect = {x + 10, y + 10, x + paletteW - 10, y + 36};
    HBRUSH inputBg = CreateSolidBrush(RGB(245, 245, 245));
    FillRect(hdc, &inputRect, inputBg);
    DeleteObject(inputBg);

    SetTextColor(hdc, m_query.empty() ? RGB(180, 180, 180) : RGB(30, 30, 30));
    RECT textRect = {x + 16, y + 12, x + paletteW - 16, y + 34};
    std::wstring displayText = m_query.empty() ? L"Type a command..." : m_query;
    DrawTextW(hdc, displayText.c_str(), -1, &textRect, DT_LEFT | DT_SINGLELINE);

    // Results
    int resultY = y + 44;
    for (int i = 0; i < static_cast<int>(m_results.size()) && resultY < bgRect.bottom - 10; ++i) {
        RECT itemRect = {x + 4, resultY, x + paletteW - 4, resultY + 28};

        if (i == m_selectedIndex) {
            HBRUSH selBrush = CreateSolidBrush(RGB(0, 120, 215));
            FillRect(hdc, &itemRect, selBrush);
            DeleteObject(selBrush);
            SetTextColor(hdc, RGB(255, 255, 255));
        } else {
            SetTextColor(hdc, RGB(30, 30, 30));
        }

        RECT nameRect = {x + 14, resultY + 4, x + paletteW - 14, resultY + 24};
        DrawTextW(hdc, m_results[i].name.c_str(), -1, &nameRect,
                  DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

        resultY += 28;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

} // namespace docvision
