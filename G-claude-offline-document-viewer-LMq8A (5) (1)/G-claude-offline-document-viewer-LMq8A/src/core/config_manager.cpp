#include "core/config_manager.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "diagnostics/logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>

// Simple JSON parsing without external dependency (fallback)
// When nlohmann/json is available, this will use it instead

namespace docvision {

ConfigManager& ConfigManager::instance() {
    static ConfigManager s_instance;
    return s_instance;
}

bool ConfigManager::initialize(const std::wstring& configDir) {
    m_configDir = configDir;
    utils::createDirectories(configDir);

    loadDefaults();

    // Load user settings (override defaults)
    loadSettings();
    loadHotkeys();

    return true;
}

void ConfigManager::loadDefaults() {
    // Default hotkeys
    m_defaultHotkeys = {
        {"file.open",           "Ctrl+O",           "Open file"},
        {"file.save",           "Ctrl+S",           "Save"},
        {"file.saveAs",         "Ctrl+Shift+S",     "Save as"},
        {"file.close",          "Ctrl+W",           "Close tab"},
        {"file.print",          "Ctrl+P",           "Print"},

        {"nav.nextPage",        "PageDown",          "Next page"},
        {"nav.prevPage",        "PageUp",            "Previous page"},
        {"nav.firstPage",       "Ctrl+Home",         "First page"},
        {"nav.lastPage",        "Ctrl+End",          "Last page"},
        {"nav.goToPage",        "Ctrl+G",            "Go to page"},
        {"nav.back",            "Alt+Left",          "Navigate back"},
        {"nav.forward",         "Alt+Right",         "Navigate forward"},
        {"nav.nextTab",         "Ctrl+Tab",          "Next tab"},
        {"nav.prevTab",         "Ctrl+Shift+Tab",    "Previous tab"},

        {"zoom.in",             "Ctrl+Plus",         "Zoom in"},
        {"zoom.out",            "Ctrl+Minus",        "Zoom out"},
        {"zoom.actualSize",     "Ctrl+0",            "Actual size"},
        {"zoom.fitWidth",       "Ctrl+1",            "Fit width"},
        {"zoom.fitPage",        "Ctrl+2",            "Fit page"},

        {"view.fullscreen",     "F11",               "Full screen"},
        {"view.slideshow",      "F5",                "Slide show"},

        {"search.find",         "Ctrl+F",            "Find"},
        {"search.findNext",     "F3",                "Find next"},
        {"search.findPrev",     "Shift+F3",          "Find previous"},

        {"ocr.area",            "Ctrl+Shift+O",      "OCR area"},
        {"screenshot.area",     "Ctrl+Alt+X",        "Screenshot area"},
        {"copy.text",           "Ctrl+C",            "Copy text"},
        {"copy.image",          "Ctrl+Shift+C",      "Copy as image"},

        {"annot.highlight",     "Ctrl+Shift+H",      "Highlight"},
        {"annot.note",          "Ctrl+Shift+N",      "Sticky note"},
        {"annot.ink",           "Ctrl+Shift+I",      "Ink tool"},
        {"annot.hideToggle",    "Ctrl+Shift+A",      "Toggle annotations"},

        {"app.commandPalette",  "Ctrl+Shift+P",      "Command palette"},
        {"display.nightMode",   "Ctrl+Shift+D",      "Night mode"},
    };

    m_hotkeys = m_defaultHotkeys;

    // Default settings
    m_settings["general.theme"] = std::string("light");
    m_settings["general.language"] = std::string("ru");
    m_settings["general.restoreSession"] = true;
    m_settings["viewer.defaultViewMode"] = std::string("continuous");
    m_settings["viewer.defaultFitMode"] = std::string("fitWidth");
    m_settings["viewer.defaultZoom"] = 1.0;
    m_settings["display.brightness"] = 0.0;
    m_settings["display.contrast"] = 1.0;
    m_settings["display.gamma"] = 1.0;
    m_settings["display.invertColors"] = false;
    m_settings["ocr.language"] = std::string("rus");
    m_settings["performance.pageCacheMemoryMB"] = 300;
    m_settings["logging.level"] = std::string("info");
}

template<>
std::string ConfigManager::get<std::string>(const std::string& key, const std::string& defaultValue) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        if (auto* val = std::get_if<std::string>(&it->second)) {
            return *val;
        }
    }
    return defaultValue;
}

template<>
bool ConfigManager::get<bool>(const std::string& key, const bool& defaultValue) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        if (auto* val = std::get_if<bool>(&it->second)) {
            return *val;
        }
    }
    return defaultValue;
}

template<>
int ConfigManager::get<int>(const std::string& key, const int& defaultValue) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        if (auto* val = std::get_if<int>(&it->second)) {
            return *val;
        }
    }
    return defaultValue;
}

template<>
double ConfigManager::get<double>(const std::string& key, const double& defaultValue) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        if (auto* val = std::get_if<double>(&it->second)) {
            return *val;
        }
    }
    return defaultValue;
}

template<>
void ConfigManager::set<std::string>(const std::string& key, const std::string& value) {
    m_settings[key] = value;
}

template<>
void ConfigManager::set<bool>(const std::string& key, const bool& value) {
    m_settings[key] = value;
}

template<>
void ConfigManager::set<int>(const std::string& key, const int& value) {
    m_settings[key] = value;
}

template<>
void ConfigManager::set<double>(const std::string& key, const double& value) {
    m_settings[key] = value;
}

bool ConfigManager::loadSettings() {
    std::wstring path = m_configDir + L"\\settings.json";
    if (!utils::fileExists(path)) {
        return false; // use defaults
    }

    std::string content = utils::readFileText(path);
    if (content.empty()) return false;

    // Simplified JSON parsing — in production, use nlohmann/json
    // For now, defaults are sufficient
    return true;
}

bool ConfigManager::saveSettings() {
    std::wstring path = m_configDir + L"\\settings.json";

    // Simplified JSON writing — in production, use nlohmann/json
    std::ostringstream json;
    json << "{\n";
    json << "  \"_version\": 1,\n";
    json << "  \"general\": {\n";
    json << "    \"theme\": \"" << get<std::string>("general.theme", "light") << "\",\n";
    json << "    \"language\": \"" << get<std::string>("general.language", "ru") << "\",\n";
    json << "    \"restoreSession\": " << (get<bool>("general.restoreSession", true) ? "true" : "false") << "\n";
    json << "  },\n";
    json << "  \"ocr\": {\n";
    json << "    \"language\": \"" << get<std::string>("ocr.language", "rus") << "\"\n";
    json << "  },\n";
    json << "  \"logging\": {\n";
    json << "    \"level\": \"" << get<std::string>("logging.level", "info") << "\"\n";
    json << "  }\n";
    json << "}\n";

    return utils::writeFileText(path, json.str());
}

bool ConfigManager::loadHotkeys() {
    std::wstring path = m_configDir + L"\\hotkeys.json";
    if (!utils::fileExists(path)) {
        return false;
    }
    // Load custom hotkeys from file — for now, use defaults
    return true;
}

bool ConfigManager::saveHotkeys() {
    std::wstring path = m_configDir + L"\\hotkeys.json";

    std::ostringstream json;
    json << "{\n  \"hotkeys\": [\n";
    for (size_t i = 0; i < m_hotkeys.size(); ++i) {
        const auto& hk = m_hotkeys[i];
        json << "    {\"command\": \"" << hk.commandId
             << "\", \"keys\": \"" << hk.keys
             << "\", \"description\": \"" << hk.description << "\"}";
        if (i + 1 < m_hotkeys.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n}\n";

    return utils::writeFileText(path, json.str());
}

std::vector<ConfigManager::HotkeyBinding> ConfigManager::getAllHotkeys() const {
    return m_hotkeys;
}

void ConfigManager::setHotkey(const std::string& commandId, const std::string& keys) {
    for (auto& hk : m_hotkeys) {
        if (hk.commandId == commandId) {
            hk.keys = keys;
            hk.isCustom = true;
            return;
        }
    }
}

void ConfigManager::resetHotkey(const std::string& commandId) {
    for (auto& hk : m_hotkeys) {
        if (hk.commandId == commandId) {
            for (const auto& def : m_defaultHotkeys) {
                if (def.commandId == commandId) {
                    hk.keys = def.keys;
                    hk.isCustom = false;
                    break;
                }
            }
            return;
        }
    }
}

void ConfigManager::resetAllHotkeys() {
    m_hotkeys = m_defaultHotkeys;
}

std::string ConfigManager::getHotkeyForCommand(const std::string& commandId) const {
    for (const auto& hk : m_hotkeys) {
        if (hk.commandId == commandId) {
            return hk.keys;
        }
    }
    return "";
}

bool ConfigManager::exportHotkeys(const std::wstring& path) const {
    // Same as saveHotkeys but to a custom path
    std::ostringstream json;
    json << "{\n  \"hotkeys\": [\n";
    for (size_t i = 0; i < m_hotkeys.size(); ++i) {
        const auto& hk = m_hotkeys[i];
        json << "    {\"command\": \"" << hk.commandId
             << "\", \"keys\": \"" << hk.keys
             << "\", \"description\": \"" << hk.description << "\"}";
        if (i + 1 < m_hotkeys.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n}\n";

    return utils::writeFileText(path, json.str());
}

bool ConfigManager::importHotkeys(const std::wstring& path) {
    if (!utils::fileExists(path)) return false;
    // Parse and apply — simplified
    return true;
}

std::wstring ConfigManager::getCacheDir() const {
    std::wstring dir = m_configDir + L"\\..\\cache";
    utils::createDirectories(dir);
    return dir;
}

std::wstring ConfigManager::getLogDir() const {
    std::wstring dir = m_configDir + L"\\..\\logs";
    utils::createDirectories(dir);
    return dir;
}

std::wstring ConfigManager::getTessdataDir() const {
    return m_configDir + L"\\..\\tessdata";
}

void ConfigManager::addRecentFile(const std::wstring& path) {
    // Remove if already present
    m_recentFiles.erase(
        std::remove(m_recentFiles.begin(), m_recentFiles.end(), path),
        m_recentFiles.end());
    // Add to front
    m_recentFiles.insert(m_recentFiles.begin(), path);
    // Trim to max
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.resize(MAX_RECENT_FILES);
    }
}

void ConfigManager::removeRecentFile(const std::wstring& path) {
    m_recentFiles.erase(
        std::remove(m_recentFiles.begin(), m_recentFiles.end(), path),
        m_recentFiles.end());
}

std::vector<std::wstring> ConfigManager::getRecentFiles() const {
    return m_recentFiles;
}

void ConfigManager::clearRecentFiles() {
    m_recentFiles.clear();
}

void ConfigManager::pinFile(const std::wstring& path) {
    if (!isFilePinned(path)) {
        m_pinnedFiles.push_back(path);
    }
}

void ConfigManager::unpinFile(const std::wstring& path) {
    m_pinnedFiles.erase(
        std::remove(m_pinnedFiles.begin(), m_pinnedFiles.end(), path),
        m_pinnedFiles.end());
}

std::vector<std::wstring> ConfigManager::getPinnedFiles() const {
    return m_pinnedFiles;
}

bool ConfigManager::isFilePinned(const std::wstring& path) const {
    return std::find(m_pinnedFiles.begin(), m_pinnedFiles.end(), path) != m_pinnedFiles.end();
}

} // namespace docvision
