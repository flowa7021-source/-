#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace docvision {

class IDocument;
class ViewportManager;
class RenderPipeline;
class PageCache;
class SelectionManager;
class NavigationHistory;

// Document tab state
struct DocumentTab {
    int id = 0;
    std::wstring filePath;
    std::wstring title;
    bool isModified = false;

    std::unique_ptr<IDocument> document;
    std::unique_ptr<ViewportManager> viewport;
    std::unique_ptr<RenderPipeline> renderPipeline;
    std::unique_ptr<PageCache> pageCache;
    std::unique_ptr<SelectionManager> selectionManager;
    std::unique_ptr<NavigationHistory> navHistory;
};

// Tab manager — multi-document tab interface
class TabManager {
public:
    TabManager();
    ~TabManager();

    bool initialize(HWND parentHwnd);

    // Tab operations
    int addTab(const std::wstring& filePath);
    void closeTab(int tabId);
    void closeAllTabs();
    void selectTab(int tabId);
    int getSelectedTabId() const { return m_selectedTabId; }
    DocumentTab* getSelectedTab();
    DocumentTab* getTab(int tabId);

    // Tab navigation
    void selectNextTab();
    void selectPreviousTab();
    int getTabCount() const { return static_cast<int>(m_tabs.size()); }
    bool hasOpenTabs() const { return !m_tabs.empty(); }

    // Tab state
    void setTabModified(int tabId, bool modified);
    void updateTabTitle(int tabId, const std::wstring& title);

    // Find tab by file path
    int findTabByPath(const std::wstring& path) const;

    // Session save/restore
    struct SessionData {
        struct TabState {
            std::wstring filePath;
            int currentPage = 0;
            double scrollX = 0, scrollY = 0;
            double zoom = 1.0;
        };
        std::vector<TabState> tabs;
        int activeTabIndex = 0;
    };

    SessionData getSessionData() const;
    void restoreSession(const SessionData& session);

    // Callbacks
    using TabChangedCallback = std::function<void(int tabId)>;
    void onTabChanged(TabChangedCallback cb) { m_onTabChanged = cb; }
    void onTabClosed(TabChangedCallback cb) { m_onTabClosed = cb; }
    void onAllTabsClosed(std::function<void()> cb) { m_onAllTabsClosed = cb; }

    // Paint
    void paint(HDC hdc, const RECT& rect);
    int getTabBarHeight() const { return m_tabBarHeight; }

    // Hit testing
    int hitTestTab(int x, int y) const;
    bool hitTestCloseButton(int tabId, int x, int y) const;

    // Mouse events
    void onMouseDown(int x, int y, bool isMiddleButton = false);
    void onMouseUp(int x, int y);
    void onMouseMove(int x, int y);

private:
    void layoutTabs();

    HWND m_parentHwnd = nullptr;
    std::vector<std::unique_ptr<DocumentTab>> m_tabs;
    int m_selectedTabId = -1;
    int m_nextTabId = 1;
    int m_tabBarHeight = 30;

    // Drag state
    int m_dragTabId = -1;
    int m_dragStartX = 0;

    TabChangedCallback m_onTabChanged;
    TabChangedCallback m_onTabClosed;
    std::function<void()> m_onAllTabsClosed;
};

} // namespace docvision
