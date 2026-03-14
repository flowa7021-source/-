#include "ui/hotkey_manager.h"
#include "core/config_manager.h"
#include <sstream>
#include <algorithm>

namespace docvision {

void HotkeyManager::initialize() {
    loadDefaultBindings();

    // Load custom bindings from config
    auto bindings = ConfigManager::instance().getAllHotkeys();
    for (const auto& b : bindings) {
        Hotkey hk = parseString(b.keys);
        if (hk.virtualKey != 0) {
            bind(b.commandId, hk);
        }
    }
}

void HotkeyManager::bind(const std::string& commandId, const Hotkey& hotkey) {
    // Remove previous binding for this command
    auto oldIt = m_commandToHotkey.find(commandId);
    if (oldIt != m_commandToHotkey.end()) {
        m_hotkeyToCommand.erase(oldIt->second);
    }

    m_commandToHotkey[commandId] = hotkey;
    m_hotkeyToCommand[hotkey] = commandId;
}

void HotkeyManager::unbind(const std::string& commandId) {
    auto it = m_commandToHotkey.find(commandId);
    if (it != m_commandToHotkey.end()) {
        m_hotkeyToCommand.erase(it->second);
        m_commandToHotkey.erase(it);
    }
}

Hotkey HotkeyManager::parseString(const std::string& str) {
    Hotkey hk;
    std::string s = str;

    // Parse modifiers
    auto findAndRemove = [&s](const std::string& token) -> bool {
        auto pos = s.find(token);
        if (pos != std::string::npos) {
            s.erase(pos, token.length());
            return true;
        }
        return false;
    };

    if (findAndRemove("Ctrl+")) hk.modifiers |= MOD_CTRL_KEY;
    if (findAndRemove("Shift+")) hk.modifiers |= MOD_SHIFT_KEY;
    if (findAndRemove("Alt+")) hk.modifiers |= MOD_ALT_KEY;

    // Remove remaining whitespace
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());

    // Parse key
    if (s == "Plus" || s == "=") hk.virtualKey = VK_OEM_PLUS;
    else if (s == "Minus" || s == "-") hk.virtualKey = VK_OEM_MINUS;
    else if (s == "Comma" || s == ",") hk.virtualKey = VK_OEM_COMMA;
    else if (s == "Period" || s == ".") hk.virtualKey = VK_OEM_PERIOD;
    else if (s == "Tab") hk.virtualKey = VK_TAB;
    else if (s == "Home") hk.virtualKey = VK_HOME;
    else if (s == "End") hk.virtualKey = VK_END;
    else if (s == "Delete") hk.virtualKey = VK_DELETE;
    else if (s == "PageUp") hk.virtualKey = VK_PRIOR;
    else if (s == "PageDown") hk.virtualKey = VK_NEXT;
    else if (s == "Left") hk.virtualKey = VK_LEFT;
    else if (s == "Right") hk.virtualKey = VK_RIGHT;
    else if (s == "Up") hk.virtualKey = VK_UP;
    else if (s == "Down") hk.virtualKey = VK_DOWN;
    else if (s == "Space") hk.virtualKey = VK_SPACE;
    else if (s == "Escape") hk.virtualKey = VK_ESCAPE;
    else if (s == "Enter") hk.virtualKey = VK_RETURN;
    else if (s.length() >= 2 && s[0] == 'F') {
        int fnum = std::atoi(s.c_str() + 1);
        if (fnum >= 1 && fnum <= 12) hk.virtualKey = VK_F1 + fnum - 1;
    }
    else if (s.length() == 1 && s[0] >= 'A' && s[0] <= 'Z') {
        hk.virtualKey = s[0];
    }
    else if (s.length() == 1 && s[0] >= 'a' && s[0] <= 'z') {
        hk.virtualKey = s[0] - 32; // uppercase
    }
    else if (s.length() == 1 && s[0] >= '0' && s[0] <= '9') {
        hk.virtualKey = s[0];
    }

    return hk;
}

std::string HotkeyManager::toString(const Hotkey& hotkey) {
    std::string result;
    if (hotkey.modifiers & MOD_CTRL_KEY) result += "Ctrl+";
    if (hotkey.modifiers & MOD_SHIFT_KEY) result += "Shift+";
    if (hotkey.modifiers & MOD_ALT_KEY) result += "Alt+";

    if (hotkey.virtualKey >= 'A' && hotkey.virtualKey <= 'Z') {
        result += static_cast<char>(hotkey.virtualKey);
    } else if (hotkey.virtualKey >= '0' && hotkey.virtualKey <= '9') {
        result += static_cast<char>(hotkey.virtualKey);
    } else if (hotkey.virtualKey >= VK_F1 && hotkey.virtualKey <= VK_F12) {
        result += "F" + std::to_string(hotkey.virtualKey - VK_F1 + 1);
    } else {
        switch (hotkey.virtualKey) {
            case VK_OEM_PLUS: result += "Plus"; break;
            case VK_OEM_MINUS: result += "Minus"; break;
            case VK_TAB: result += "Tab"; break;
            case VK_HOME: result += "Home"; break;
            case VK_END: result += "End"; break;
            case VK_DELETE: result += "Delete"; break;
            case VK_PRIOR: result += "PageUp"; break;
            case VK_NEXT: result += "PageDown"; break;
            case VK_LEFT: result += "Left"; break;
            case VK_RIGHT: result += "Right"; break;
            default: result += "?"; break;
        }
    }
    return result;
}

bool HotkeyManager::processKeyEvent(UINT vkey, bool ctrlDown, bool shiftDown, bool altDown) {
    Hotkey hk;
    hk.virtualKey = vkey;
    if (ctrlDown) hk.modifiers |= MOD_CTRL_KEY;
    if (shiftDown) hk.modifiers |= MOD_SHIFT_KEY;
    if (altDown) hk.modifiers |= MOD_ALT_KEY;

    auto it = m_hotkeyToCommand.find(hk);
    if (it != m_hotkeyToCommand.end()) {
        if (m_executor) {
            m_executor(it->second);
            return true;
        }
    }
    return false;
}

Hotkey HotkeyManager::getHotkeyForCommand(const std::string& commandId) const {
    auto it = m_commandToHotkey.find(commandId);
    return it != m_commandToHotkey.end() ? it->second : Hotkey{};
}

std::string HotkeyManager::getCommandForHotkey(const Hotkey& hotkey) const {
    auto it = m_hotkeyToCommand.find(hotkey);
    return it != m_hotkeyToCommand.end() ? it->second : "";
}

bool HotkeyManager::hasConflict(const Hotkey& hotkey, const std::string& excludeCommandId) const {
    auto it = m_hotkeyToCommand.find(hotkey);
    return it != m_hotkeyToCommand.end() && it->second != excludeCommandId;
}

std::vector<HotkeyManager::HotkeyInfo> HotkeyManager::getAllBindings() const {
    std::vector<HotkeyInfo> result;
    for (const auto& [cmdId, hk] : m_commandToHotkey) {
        result.push_back({cmdId, hk, toString(hk)});
    }
    return result;
}

void HotkeyManager::onKeyDown(UINT vkey) {
    if (vkey == VK_SPACE) m_spaceHeld = true;
}

void HotkeyManager::onKeyUp(UINT vkey) {
    if (vkey == VK_SPACE) m_spaceHeld = false;
}

void HotkeyManager::loadDefaultBindings() {
    auto bindings = ConfigManager::instance().getAllHotkeys();
    for (const auto& b : bindings) {
        Hotkey hk = parseString(b.keys);
        if (hk.virtualKey != 0) {
            m_commandToHotkey[b.commandId] = hk;
            m_hotkeyToCommand[hk] = b.commandId;
        }
    }
}

} // namespace docvision
