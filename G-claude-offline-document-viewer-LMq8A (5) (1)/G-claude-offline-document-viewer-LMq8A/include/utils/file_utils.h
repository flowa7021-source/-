#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace docvision {
namespace utils {

bool fileExists(const std::wstring& path);
bool directoryExists(const std::wstring& path);
bool createDirectories(const std::wstring& path);

int64_t getFileSize(const std::wstring& path);
std::wstring getLastModified(const std::wstring& path);

std::vector<uint8_t> readFileBytes(const std::wstring& path);
bool writeFileBytes(const std::wstring& path, const std::vector<uint8_t>& data);
std::string readFileText(const std::wstring& path);
bool writeFileText(const std::wstring& path, const std::string& text);

std::wstring getExeDirectory();
std::wstring getTempDirectory();

// Compute SHA-256 hash of file content
std::string computeFileHash(const std::wstring& path);

// Open file dialog
std::wstring showOpenFileDialog(void* parentHwnd,
                                 const std::wstring& filter,
                                 const std::wstring& title = L"Open File");

std::wstring showSaveFileDialog(void* parentHwnd,
                                 const std::wstring& filter,
                                 const std::wstring& defaultName,
                                 const std::wstring& title = L"Save File");

std::wstring showFolderDialog(void* parentHwnd,
                               const std::wstring& title = L"Select Folder");

// Supported file filter string for dialogs
std::wstring getSupportedFileFilter();

} // namespace utils
} // namespace docvision
