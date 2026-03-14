#include "search/search_engine.h"
#include "ocr/ocr_index.h"
#include "utils/string_utils.h"

// OCR search helper — searches through OCR index data for scanned documents

namespace docvision {
namespace detail {

std::vector<SearchResult> searchOCRIndex(const OCRIndex& index,
                                          const std::wstring& query,
                                          bool caseSensitive) {
    std::vector<SearchResult> results;

    auto pages = index.getIndexedPages();
    for (int pageIndex : pages) {
        auto blocks = index.getPageBlocks(pageIndex);
        for (const auto& block : blocks) {
            std::wstring text = block.text;
            std::wstring searchText = query;

            if (!caseSensitive) {
                text = utils::toLower(text);
                searchText = utils::toLower(searchText);
            }

            size_t pos = text.find(searchText);
            while (pos != std::wstring::npos) {
                SearchResult result;
                result.pageIndex = pageIndex;
                result.bounds = block.bounds;
                result.matchText = block.text.substr(pos, query.size());

                // Context
                size_t contextStart = (pos > 20) ? pos - 20 : 0;
                size_t contextEnd = std::min(pos + query.size() + 20, block.text.size());
                result.contextBefore = block.text.substr(contextStart, pos - contextStart);
                result.contextAfter = block.text.substr(pos + query.size(),
                                                         contextEnd - pos - query.size());

                results.push_back(result);
                pos = text.find(searchText, pos + 1);
            }
        }
    }

    return results;
}

} // namespace detail
} // namespace docvision
