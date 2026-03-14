#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace docvision {

class CommandManager;

// Command palette — quick command access (Ctrl+Shift+P)
class CommandPalette {
public:
    CommandPalette();
    ~CommandPalette();

    void initialize(HWND parentHwnd, CommandManager* commandManager);
    void show();
    void hide();
    bool isVisible() const { return m_isVisible; }

    // Handle keyboard input
    bool handleKeyDown(UINT vkey);
    void handleChar(wchar_t ch);

    // Paint
    void paint(HDC hdc, const RECT& parentRect);

private:
    void updateResults();
    void executeSelected();

    HWND m_parentHwnd = nullptr;
    CommandManager* m_commandManager = nullptr;
    bool m_isVisible = false;
    std::wstring m_query;
    int m_selectedIndex = 0;

    struct PaletteEntry {
        std::string commandId;
        std::wstring name;
        std::wstring description;
        std::string hotkey;
    };
    std::vector<PaletteEntry> m_results;
};

} // namespace docvision
