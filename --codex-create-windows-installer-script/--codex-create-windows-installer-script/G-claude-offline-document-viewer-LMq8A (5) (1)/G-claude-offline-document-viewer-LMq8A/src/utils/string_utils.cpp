#include "utils/string_utils.h"
#include <windows.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cwctype>

namespace docvision {
namespace utils {

std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), result.data(), len);
    return result;
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), result.data(), len, nullptr, nullptr);
    return result;
}

std::wstring toLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](wchar_t c) { return std::towlower(c); });
    return result;
}

std::wstring toUpper(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](wchar_t c) { return std::towupper(c); });
    return result;
}

bool containsIgnoreCase(const std::wstring& haystack, const std::wstring& needle) {
    return toLower(haystack).find(toLower(needle)) != std::wstring::npos;
}

bool startsWithIgnoreCase(const std::wstring& str, const std::wstring& prefix) {
    if (prefix.size() > str.size()) return false;
    return toLower(str.substr(0, prefix.size())) == toLower(prefix);
}

std::vector<std::wstring> split(const std::wstring& str, wchar_t delimiter) {
    std::vector<std::wstring> parts;
    std::wstringstream ss(str);
    std::wstring item;
    while (std::getline(ss, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

std::wstring join(const std::vector<std::wstring>& parts, const std::wstring& separator) {
    std::wstring result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += separator;
        result += parts[i];
    }
    return result;
}

std::wstring trim(const std::wstring& str) {
    size_t start = str.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return {};
    size_t end = str.find_last_not_of(L" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::wstring formatFileSize(int64_t bytes) {
    if (bytes < 1024) {
        return std::to_wstring(bytes) + L" B";
    } else if (bytes < 1024 * 1024) {
        double kb = bytes / 1024.0;
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << kb << L" KB";
        return ss.str();
    } else if (bytes < 1024LL * 1024 * 1024) {
        double mb = bytes / (1024.0 * 1024.0);
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << mb << L" MB";
        return ss.str();
    } else {
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(2) << gb << L" GB";
        return ss.str();
    }
}

std::wstring formatDate(const std::wstring& isoDate) {
    // Simple pass-through for now; could format to locale-specific
    return isoDate;
}

std::wstring getFileExtension(const std::wstring& path) {
    auto pos = path.rfind(L'.');
    if (pos == std::wstring::npos) return {};
    return toLower(path.substr(pos));
}

std::wstring getFileName(const std::wstring& path) {
    auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return path;
    return path.substr(pos + 1);
}

std::wstring getFileDirectory(const std::wstring& path) {
    auto pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return {};
    return path.substr(0, pos);
}

} // namespace utils
} // namespace docvision
