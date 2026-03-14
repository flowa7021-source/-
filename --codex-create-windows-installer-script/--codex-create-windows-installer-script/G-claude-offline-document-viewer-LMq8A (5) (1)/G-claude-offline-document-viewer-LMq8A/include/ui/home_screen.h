#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace docvision {

// Home screen / Library — displayed when no document is active
class HomeScreen {
public:
    HomeScreen();
    ~HomeScreen();

    bool initialize(HWND parentHwnd);

    // Visibility
    void show();
    void hide();
    bool isVisible() const { return m_isVisible; }

    // Content
    struct RecentFileEntry {
        std::wstring path;
        std::wstring title;
        std::wstring format;        // "PDF", "DjVu", etc.
        std::wstring lastOpened;    // formatted date
        bool isPinned = false;
    };

    void setRecentFiles(const std::vector<RecentFileEntry>& files);
    void setHotkeySheet(const std::vector<std::pair<std::wstring, std::wstring>>& hotkeys);

    // Quick actions
    enum class QuickAction {
        OpenFile,
        ImportFolder,
        ClearHistory,
        Settings,
        ShowHotkeys
    };

    // Callbacks
    using FileSelectedCallback = std::function<void(const std::wstring& path)>;
    void onFileSelected(FileSelectedCallback cb) { m_onFileSelected = cb; }

    using ActionCallback = std::function<void(QuickAction action)>;
    void onAction(ActionCallback cb) { m_onAction = cb; }

    using PinToggleCallback = std::function<void(const std::wstring& path, bool pinned)>;
    void onPinToggle(PinToggleCallback cb) { m_onPinToggle = cb; }

    // Paint
    void paint(HDC hdc, const RECT& rect);

    // Mouse events
    void onMouseDown(int x, int y);
    void onMouseUp(int x, int y);
    void onMouseMove(int x, int y);
    void onMouseWheel(short delta);

    // Search filter
    void setSearchFilter(const std::wstring& filter);

private:
    void paintHeader(HDC hdc, const RECT& rect);
    void paintQuickActions(HDC hdc, const RECT& rect);
    void paintRecentFiles(HDC hdc, const RECT& rect);
    void paintHotkeySheet(HDC hdc, const RECT& rect);
    void paintSearchBar(HDC hdc, const RECT& rect);

    HWND m_parentHwnd = nullptr;
    bool m_isVisible = true;
    int m_scrollOffset = 0;

    std::vector<RecentFileEntry> m_recentFiles;
    std::vector<RecentFileEntry> m_filteredFiles;
    std::vector<std::pair<std::wstring, std::wstring>> m_hotkeySheet;
    std::wstring m_searchFilter;
    bool m_showHotkeys = false;

    int m_hoveredItemIndex = -1;

    FileSelectedCallback m_onFileSelected;
    ActionCallback m_onAction;
    PinToggleCallback m_onPinToggle;
};

} // namespace docvision
