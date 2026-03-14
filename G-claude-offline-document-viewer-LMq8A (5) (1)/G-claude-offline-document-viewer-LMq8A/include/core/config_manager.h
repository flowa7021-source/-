#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>

namespace docvision {

// Configuration value types
using ConfigValue = std::variant<bool, int, double, std::string, std::wstring>;

// Configuration manager — portable JSON-based settings
class ConfigManager {
public:
    static ConfigManager& instance();

    // Initialize with path to config directory (next to exe)
    bool initialize(const std::wstring& configDir);

    // Settings
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;

    template<typename T>
    void set(const std::string& key, const T& value);

    // Save/load
    bool loadSettings();
    bool saveSettings();
    bool loadHotkeys();
    bool saveHotkeys();

    // Hotkey management
    struct HotkeyBinding {
        std::string commandId;
        std::string keys;        // e.g., "Ctrl+Alt+X"
        std::string description;
        bool isCustom = false;
    };

    std::vector<HotkeyBinding> getAllHotkeys() const;
    void setHotkey(const std::string& commandId, const std::string& keys);
    void resetHotkey(const std::string& commandId);
    void resetAllHotkeys();
    std::string getHotkeyForCommand(const std::string& commandId) const;

    // Export/import hotkeys
    bool exportHotkeys(const std::wstring& path) const;
    bool importHotkeys(const std::wstring& path);

    // Paths
    std::wstring getConfigDir() const { return m_configDir; }
    std::wstring getCacheDir() const;
    std::wstring getLogDir() const;
    std::wstring getTessdataDir() const;

    // Recent files
    void addRecentFile(const std::wstring& path);
    void removeRecentFile(const std::wstring& path);
    std::vector<std::wstring> getRecentFiles() const;
    void clearRecentFiles();
    static constexpr int MAX_RECENT_FILES = 50;

    // Pinned files
    void pinFile(const std::wstring& path);
    void unpinFile(const std::wstring& path);
    std::vector<std::wstring> getPinnedFiles() const;
    bool isFilePinned(const std::wstring& path) const;

private:
    ConfigManager() = default;
    void loadDefaults();

    std::wstring m_configDir;
    std::unordered_map<std::string, ConfigValue> m_settings;
    std::vector<HotkeyBinding> m_hotkeys;
    std::vector<HotkeyBinding> m_defaultHotkeys;
    std::vector<std::wstring> m_recentFiles;
    std::vector<std::wstring> m_pinnedFiles;
};

} // namespace docvision
