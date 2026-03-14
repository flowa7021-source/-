#include "search/search_engine.h"
#include "utils/string_utils.h"

// Text search helper implementation
// Main search logic is in SearchEngine; this module provides utilities
// for text matching within extracted page content.

namespace docvision {
namespace detail {

bool matchWord(const std::wstring& text, size_t pos, size_t len) {
    if (pos > 0) {
        wchar_t c = text[pos - 1];
        if (std::iswalnum(c) || c == L'_') return false;
    }
    size_t end = pos + len;
    if (end < text.size()) {
        wchar_t c = text[end];
        if (std::iswalnum(c) || c == L'_') return false;
    }
    return true;
}

std::vector<size_t> findAllOccurrences(const std::wstring& text,
                                        const std::wstring& needle,
                                        bool caseSensitive,
                                        bool wholeWord) {
    std::vector<size_t> positions;
    if (needle.empty() || text.empty()) return positions;

    std::wstring haystack = caseSensitive ? text : utils::toLower(text);
    std::wstring search = caseSensitive ? needle : utils::toLower(needle);

    size_t pos = 0;
    while ((pos = haystack.find(search, pos)) != std::wstring::npos) {
        if (!wholeWord || matchWord(haystack, pos, search.size())) {
            positions.push_back(pos);
        }
        pos += 1;
    }
    return positions;
}

} // namespace detail
} // namespace docvision
