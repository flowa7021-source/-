#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace docvision {

// Command definition
struct Command {
    std::string id;
    std::wstring name;
    std::wstring description;
    std::function<void()> execute;
    std::function<bool()> isEnabled;
    std::function<bool()> isChecked;
    std::string category;
};

// Command manager — central command routing and command palette support
class CommandManager {
public:
    CommandManager() = default;

    // Register commands
    void registerCommand(const Command& command);
    void unregisterCommand(const std::string& commandId);

    // Execute
    bool executeCommand(const std::string& commandId);

    // Query
    bool isCommandEnabled(const std::string& commandId) const;
    bool isCommandChecked(const std::string& commandId) const;
    const Command* getCommand(const std::string& commandId) const;

    // List all commands (for command palette)
    std::vector<const Command*> getAllCommands() const;
    std::vector<const Command*> searchCommands(const std::wstring& query) const;

    // Categories
    std::vector<std::string> getCategories() const;
    std::vector<const Command*> getCommandsByCategory(const std::string& category) const;

private:
    std::unordered_map<std::string, Command> m_commands;
};

} // namespace docvision
