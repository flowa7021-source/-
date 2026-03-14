#include "library/session_manager.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "diagnostics/logger.h"
#include <fstream>
#include <sstream>

namespace docvision {

bool SessionManager::initialize(const std::wstring& sessionFilePath) {
    m_filePath = sessionFilePath;
    return true;
}

bool SessionManager::saveSession(const SessionState& state) {
    if (m_filePath.empty()) return false;

    // Build JSON manually (no dependency on nlohmann_json at link time)
    std::ostringstream json;
    json << "{\n";
    json << "  \"activeTabIndex\": " << state.activeTabIndex << ",\n";
    json << "  \"windowX\": " << state.windowX << ",\n";
    json << "  \"windowY\": " << state.windowY << ",\n";
    json << "  \"windowWidth\": " << state.windowWidth << ",\n";
    json << "  \"windowHeight\": " << state.windowHeight << ",\n";
    json << "  \"windowMaximized\": " << (state.windowMaximized ? "true" : "false") << ",\n";
    json << "  \"sidePanelMode\": " << state.sidePanelMode << ",\n";
    json << "  \"sidePanelWidth\": " << state.sidePanelWidth << ",\n";
    json << "  \"tabs\": [\n";

    for (size_t i = 0; i < state.tabs.size(); ++i) {
        const auto& tab = state.tabs[i];
        json << "    {\n";
        json << "      \"filePath\": \"" << utils::wideToUtf8(tab.filePath) << "\",\n";
        json << "      \"currentPage\": " << tab.currentPage << ",\n";
        json << "      \"scrollX\": " << tab.scrollX << ",\n";
        json << "      \"scrollY\": " << tab.scrollY << ",\n";
        json << "      \"zoom\": " << tab.zoom << ",\n";
        json << "      \"viewMode\": " << tab.viewMode << "\n";
        json << "    }";
        if (i + 1 < state.tabs.size()) json << ",";
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";

    return utils::writeFileText(m_filePath, json.str());
}

SessionState SessionManager::loadSession() const {
    SessionState state;
    if (m_filePath.empty()) return state;

    std::string content = utils::readFileText(m_filePath);
    if (content.empty()) return state;

    // Simple JSON parsing for session data
    auto extractInt = [&](const std::string& key) -> int {
        auto pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return 0;
        pos = content.find(':', pos);
        if (pos == std::string::npos) return 0;
        return std::stoi(content.substr(pos + 1));
    };

    auto extractBool = [&](const std::string& key) -> bool {
        auto pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        pos = content.find(':', pos);
        if (pos == std::string::npos) return false;
        return content.find("true", pos) < content.find('\n', pos);
    };

    state.activeTabIndex = extractInt("activeTabIndex");
    state.windowX = extractInt("windowX");
    state.windowY = extractInt("windowY");
    state.windowWidth = extractInt("windowWidth");
    state.windowHeight = extractInt("windowHeight");
    state.windowMaximized = extractBool("windowMaximized");
    state.sidePanelMode = extractInt("sidePanelMode");
    state.sidePanelWidth = extractInt("sidePanelWidth");

    return state;
}

bool SessionManager::hasSession() const {
    return !m_filePath.empty() && utils::fileExists(m_filePath);
}

void SessionManager::clearSession() {
    if (!m_filePath.empty()) {
        utils::writeFileText(m_filePath, "{}");
    }
}

} // namespace docvision
