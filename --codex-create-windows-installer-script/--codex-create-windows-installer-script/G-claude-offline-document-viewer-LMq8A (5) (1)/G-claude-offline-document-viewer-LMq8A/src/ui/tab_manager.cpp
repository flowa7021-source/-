#include "ui/tab_manager.h"
#include "core/document.h"
#include "core/viewport_manager.h"
#include "core/render_pipeline.h"
#include "core/page_cache.h"
#include "core/selection_manager.h"
#include "core/navigation_history.h"
#include "core/config_manager.h"
#include "utils/string_utils.h"
#include "diagnostics/logger.h"

namespace docvision {

TabManager::TabManager() {}
TabManager::~TabManager() {}

bool TabManager::initialize(HWND parentHwnd) {
    m_parentHwnd = parentHwnd;
    return true;
}

int TabManager::addTab(const std::wstring& filePath) {
    auto doc = DocumentFactory::createFromFile(filePath);
    if (!doc) {
        LOG_ERROR("Failed to open document: " + utils::wideToUtf8(filePath));
        return -1;
    }

    auto tab = std::make_unique<DocumentTab>();
    tab->id = m_nextTabId++;
    tab->filePath = filePath;
    tab->title = utils::getFileName(filePath);
    tab->document = std::move(doc);
    tab->viewport = std::make_unique<ViewportManager>();
    tab->renderPipeline = std::make_unique<RenderPipeline>();
    tab->pageCache = std::make_unique<PageCache>(
        ConfigManager::instance().get<int>("performance.pageCacheMemoryMB", 300) * 1024 * 1024);
    tab->selectionManager = std::make_unique<SelectionManager>();
    tab->navHistory = std::make_unique<NavigationHistory>();

    // Initialize viewport
    tab->viewport->setPageCount(tab->document->getPageCount());
    tab->renderPipeline->setDocument(tab->document.get());

    int tabId = tab->id;
    m_tabs.push_back(std::move(tab));

    LOG_INFO("Tab created: " + utils::wideToUtf8(filePath) +
             " (" + std::to_string(m_tabs.back()->document->getPageCount()) + " pages)");

    selectTab(tabId);
    return tabId;
}

void TabManager::closeTab(int tabId) {
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
                            [tabId](const auto& t) { return t->id == tabId; });
    if (it == m_tabs.end()) return;

    // Cleanup
    (*it)->renderPipeline->shutdown();
    (*it)->pageCache->clear();

    bool wasSelected = (tabId == m_selectedTabId);
    m_tabs.erase(it);

    if (m_onTabClosed) m_onTabClosed(tabId);

    if (m_tabs.empty()) {
        m_selectedTabId = -1;
        if (m_onAllTabsClosed) m_onAllTabsClosed();
    } else if (wasSelected) {
        selectTab(m_tabs.back()->id);
    }
}

void TabManager::closeAllTabs() {
    for (auto& tab : m_tabs) {
        tab->renderPipeline->shutdown();
        tab->pageCache->clear();
    }
    m_tabs.clear();
    m_selectedTabId = -1;
    if (m_onAllTabsClosed) m_onAllTabsClosed();
}

void TabManager::selectTab(int tabId) {
    if (m_selectedTabId == tabId) return;
    m_selectedTabId = tabId;
    if (m_onTabChanged) m_onTabChanged(tabId);
    InvalidateRect(m_parentHwnd, nullptr, TRUE);
}

DocumentTab* TabManager::getSelectedTab() {
    return getTab(m_selectedTabId);
}

DocumentTab* TabManager::getTab(int tabId) {
    for (auto& tab : m_tabs) {
        if (tab->id == tabId) return tab.get();
    }
    return nullptr;
}

void TabManager::selectNextTab() {
    if (m_tabs.size() <= 1) return;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i]->id == m_selectedTabId) {
            size_t next = (i + 1) % m_tabs.size();
            selectTab(m_tabs[next]->id);
            return;
        }
    }
}

void TabManager::selectPreviousTab() {
    if (m_tabs.size() <= 1) return;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i]->id == m_selectedTabId) {
            size_t prev = (i == 0) ? m_tabs.size() - 1 : i - 1;
            selectTab(m_tabs[prev]->id);
            return;
        }
    }
}

void TabManager::setTabModified(int tabId, bool modified) {
    if (auto* tab = getTab(tabId)) {
        tab->isModified = modified;
    }
}

void TabManager::updateTabTitle(int tabId, const std::wstring& title) {
    if (auto* tab = getTab(tabId)) {
        tab->title = title;
    }
}

int TabManager::findTabByPath(const std::wstring& path) const {
    for (const auto& tab : m_tabs) {
        if (tab->filePath == path) return tab->id;
    }
    return -1;
}

TabManager::SessionData TabManager::getSessionData() const {
    SessionData data;
    for (const auto& tab : m_tabs) {
        SessionData::TabState ts;
        ts.filePath = tab->filePath;
        ts.currentPage = tab->viewport->getCurrentPage();
        ts.scrollX = tab->viewport->getScrollX();
        ts.scrollY = tab->viewport->getScrollY();
        ts.zoom = tab->viewport->getZoom();
        data.tabs.push_back(ts);
    }
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i]->id == m_selectedTabId) {
            data.activeTabIndex = static_cast<int>(i);
            break;
        }
    }
    return data;
}

void TabManager::restoreSession(const SessionData& session) {
    for (const auto& tab : session.tabs) {
        int tabId = addTab(tab.filePath);
        if (tabId >= 0) {
            auto* t = getTab(tabId);
            t->viewport->setCurrentPage(tab.currentPage);
            t->viewport->setZoom(tab.zoom);
            t->viewport->setScrollPosition(tab.scrollX, tab.scrollY);
        }
    }
    if (session.activeTabIndex < static_cast<int>(m_tabs.size())) {
        selectTab(m_tabs[session.activeTabIndex]->id);
    }
}

void TabManager::paint(HDC hdc, const RECT& rect) {
    // Draw tab bar background
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    int tabX = 0;
    int tabWidth = 200;
    int tabH = m_tabBarHeight;

    for (const auto& tab : m_tabs) {
        RECT tabRect = {tabX, rect.top, tabX + tabWidth, rect.top + tabH};

        if (tab->id == m_selectedTabId) {
            HBRUSH selBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &tabRect, selBrush);
            DeleteObject(selBrush);
        }

        std::wstring displayTitle = tab->title;
        if (tab->isModified) displayTitle = L"* " + displayTitle;

        RECT textRect = tabRect;
        textRect.left += 8;
        textRect.right -= 24; // room for close button
        DrawTextW(hdc, displayTitle.c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // Close button "x"
        RECT closeRect = {tabRect.right - 20, tabRect.top + 6,
                           tabRect.right - 6, tabRect.top + tabH - 6};
        SetTextColor(hdc, RGB(128, 128, 128));
        DrawTextW(hdc, L"\u00D7", -1, &closeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        tabX += tabWidth + 2;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

int TabManager::hitTestTab(int x, int /*y*/) const {
    int tabX = 0;
    int tabWidth = 200;
    for (const auto& tab : m_tabs) {
        if (x >= tabX && x < tabX + tabWidth) {
            return tab->id;
        }
        tabX += tabWidth + 2;
    }
    return -1;
}

bool TabManager::hitTestCloseButton(int tabId, int x, int /*y*/) const {
    int tabX = 0;
    int tabWidth = 200;
    for (const auto& tab : m_tabs) {
        if (tab->id == tabId) {
            return x >= tabX + tabWidth - 20 && x < tabX + tabWidth;
        }
        tabX += tabWidth + 2;
    }
    return false;
}

void TabManager::onMouseDown(int x, int y, bool isMiddleButton) {
    int tabId = hitTestTab(x, y);
    if (tabId < 0) return;

    if (isMiddleButton || hitTestCloseButton(tabId, x, y)) {
        closeTab(tabId);
    } else {
        selectTab(tabId);
    }
}

void TabManager::onMouseUp(int /*x*/, int /*y*/) {}

void TabManager::onMouseMove(int /*x*/, int /*y*/) {}

} // namespace docvision
