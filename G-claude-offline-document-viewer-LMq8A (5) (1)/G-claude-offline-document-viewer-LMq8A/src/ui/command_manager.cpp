#include "ui/command_manager.h"
#include "utils/string_utils.h"
#include <algorithm>

namespace docvision {

void CommandManager::registerCommand(const Command& command) {
    m_commands[command.id] = command;
}

void CommandManager::unregisterCommand(const std::string& commandId) {
    m_commands.erase(commandId);
}

bool CommandManager::executeCommand(const std::string& commandId) {
    auto it = m_commands.find(commandId);
    if (it == m_commands.end()) return false;
    if (it->second.isEnabled && !it->second.isEnabled()) return false;
    if (it->second.execute) {
        it->second.execute();
        return true;
    }
    return false;
}

bool CommandManager::isCommandEnabled(const std::string& commandId) const {
    auto it = m_commands.find(commandId);
    if (it == m_commands.end()) return false;
    if (it->second.isEnabled) return it->second.isEnabled();
    return true;
}

bool CommandManager::isCommandChecked(const std::string& commandId) const {
    auto it = m_commands.find(commandId);
    if (it == m_commands.end()) return false;
    if (it->second.isChecked) return it->second.isChecked();
    return false;
}

const Command* CommandManager::getCommand(const std::string& commandId) const {
    auto it = m_commands.find(commandId);
    return it != m_commands.end() ? &it->second : nullptr;
}

std::vector<const Command*> CommandManager::getAllCommands() const {
    std::vector<const Command*> result;
    for (const auto& [id, cmd] : m_commands) {
        result.push_back(&cmd);
    }
    return result;
}

std::vector<const Command*> CommandManager::searchCommands(const std::wstring& query) const {
    std::vector<const Command*> result;
    std::wstring lowerQuery = utils::toLower(query);
    for (const auto& [id, cmd] : m_commands) {
        if (utils::containsIgnoreCase(cmd.name, query) ||
            utils::containsIgnoreCase(cmd.description, query)) {
            result.push_back(&cmd);
        }
    }
    return result;
}

std::vector<std::string> CommandManager::getCategories() const {
    std::vector<std::string> categories;
    for (const auto& [id, cmd] : m_commands) {
        if (std::find(categories.begin(), categories.end(), cmd.category) == categories.end()) {
            categories.push_back(cmd.category);
        }
    }
    return categories;
}

std::vector<const Command*> CommandManager::getCommandsByCategory(const std::string& category) const {
    std::vector<const Command*> result;
    for (const auto& [id, cmd] : m_commands) {
        if (cmd.category == category) result.push_back(&cmd);
    }
    return result;
}

} // namespace docvision
