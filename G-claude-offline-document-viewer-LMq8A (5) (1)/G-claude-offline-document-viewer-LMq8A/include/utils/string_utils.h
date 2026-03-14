#pragma once

#include <string>
#include <vector>

namespace docvision {
namespace utils {

std::wstring utf8ToWide(const std::string& utf8);
std::string wideToUtf8(const std::wstring& wide);

std::wstring toLower(const std::wstring& str);
std::wstring toUpper(const std::wstring& str);

bool containsIgnoreCase(const std::wstring& haystack, const std::wstring& needle);
bool startsWithIgnoreCase(const std::wstring& str, const std::wstring& prefix);

std::vector<std::wstring> split(const std::wstring& str, wchar_t delimiter);
std::wstring join(const std::vector<std::wstring>& parts, const std::wstring& separator);
std::wstring trim(const std::wstring& str);

std::wstring formatFileSize(int64_t bytes);
std::wstring formatDate(const std::wstring& isoDate);

std::wstring getFileExtension(const std::wstring& path);
std::wstring getFileName(const std::wstring& path);
std::wstring getFileDirectory(const std::wstring& path);

} // namespace utils
} // namespace docvision
