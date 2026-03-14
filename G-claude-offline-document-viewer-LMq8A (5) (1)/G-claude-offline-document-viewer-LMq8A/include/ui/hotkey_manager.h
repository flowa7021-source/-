#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace docvision {

// Hotkey modifier flags
enum HotkeyModifier : UINT {
    MOD_NONE = 0,
    MOD_CTRL_KEY = 0x01,
    MOD_SHIFT_KEY = 0x02,
    MOD_ALT_KEY = 0x04,
    MOD_WIN_KEY = 0x08
};

// Parsed hotkey
struct Hotkey {
    UINT modifiers = MOD_NONE;
    UINT virtualKey = 0;

    bool operator==(const Hotkey& other) const {
        return modifiers == other.modifiers && virtualKey == other.virtualKey;
    }
};

struct HotkeyHash {
    size_t operator()(const Hotkey& hk) const {
        return std::hash<UINT>()(hk.modifiers) ^ (std::hash<UINT>()(hk.virtualKey) << 16);
    }
};

// Hotkey manager — configurable keyboard shortcuts
class HotkeyManager {
public:
    HotkeyManager() = default;

    void initialize();

    // Bind hotkey to command
    void bind(const std::string& commandId, const Hotkey& hotkey);
    void unbind(const std::string& commandId);

    // Parse hotkey string (e.g., "Ctrl+Alt+X")
    static Hotkey parseString(const std::string& str);
    static std::string toString(const Hotkey& hotkey);

    // Process key event — returns true if handled
    bool processKeyEvent(UINT vkey, bool ctrlDown, bool shiftDown, bool altDown);

    // Query
    Hotkey getHotkeyForCommand(const std::string& commandId) const;
    std::string getCommandForHotkey(const Hotkey& hotkey) const;

    // Conflict detection
    bool hasConflict(const Hotkey& hotkey, const std::string& excludeCommandId = "") const;

    // List all bindings
    struct HotkeyInfo {
        std::string commandId;
        Hotkey hotkey;
        std::string hotkeyString;
    };
    std::vector<HotkeyInfo> getAllBindings() const;

    // Command execution callback
    using CommandExecutor = std::function<void(const std::string& commandId)>;
    void setCommandExecutor(CommandExecutor executor) { m_executor = executor; }

    // Temporary "Space held = hand tool" support
    void onKeyDown(UINT vkey);
    void onKeyUp(UINT vkey);
    bool isSpaceHeld() const { return m_spaceHeld; }

private:
    void loadDefaultBindings();

    std::unordered_map<std::string, Hotkey> m_commandToHotkey;
    std::unordered_map<Hotkey, std::string, HotkeyHash> m_hotkeyToCommand;
    CommandExecutor m_executor;
    bool m_spaceHeld = false;
};

} // namespace docvision
