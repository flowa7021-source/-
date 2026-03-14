#pragma once

#include <string>
#include <vector>

namespace docvision {

// Session data for save/restore
struct SessionState {
    struct TabState {
        std::wstring filePath;
        int currentPage = 0;
        double scrollX = 0;
        double scrollY = 0;
        double zoom = 1.0;
        int viewMode = 0;
    };

    std::vector<TabState> tabs;
    int activeTabIndex = 0;
    int windowX = 0, windowY = 0;
    int windowWidth = 1024, windowHeight = 768;
    bool windowMaximized = false;
    int sidePanelMode = 0;
    int sidePanelWidth = 250;
};

// Session manager — save/restore application state
class SessionManager {
public:
    SessionManager() = default;

    bool initialize(const std::wstring& sessionFilePath);

    bool saveSession(const SessionState& state);
    SessionState loadSession() const;
    bool hasSession() const;
    void clearSession();

private:
    std::wstring m_filePath;
};

} // namespace docvision
