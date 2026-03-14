#include "ocr/ocr_index.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "diagnostics/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace docvision {

OCRIndex::OCRIndex() {}
OCRIndex::~OCRIndex() {}

bool OCRIndex::initialize(const std::wstring& cacheDir) {
    m_cacheDir = cacheDir;
    utils::createDirectories(cacheDir);
    return true;
}

void OCRIndex::storePageResult(const std::string& documentHash, int pageIndex,
                                const OCRPageResult& result) {
    PageOCRData data;
    data.blocks = result.blocks;
    data.fullText = result.fullText;
    m_index[documentHash][pageIndex] = std::move(data);
}

bool OCRIndex::hasPageResult(const std::string& documentHash, int pageIndex) const {
    auto docIt = m_index.find(documentHash);
    if (docIt == m_index.end()) return false;
    return docIt->second.find(pageIndex) != docIt->second.end();
}

OCRPageResult OCRIndex::getPageResult(const std::string& documentHash, int pageIndex) const {
    OCRPageResult result;
    auto docIt = m_index.find(documentHash);
    if (docIt == m_index.end()) return result;
    auto pageIt = docIt->second.find(pageIndex);
    if (pageIt == docIt->second.end()) return result;

    result.pageIndex = pageIndex;
    result.blocks = pageIt->second.blocks;
    result.fullText = pageIt->second.fullText;
    result.success = true;
    return result;
}

std::vector<SearchResult> OCRIndex::search(const std::string& documentHash,
                                            const std::wstring& query,
                                            bool caseSensitive) const {
    std::vector<SearchResult> results;
    auto docIt = m_index.find(documentHash);
    if (docIt == m_index.end()) return results;

    std::wstring searchQuery = caseSensitive ? query : utils::toLower(query);

    for (const auto& [pageIndex, data] : docIt->second) {
        std::wstring text = caseSensitive ? data.fullText : utils::toLower(data.fullText);
        size_t pos = 0;
        while ((pos = text.find(searchQuery, pos)) != std::wstring::npos) {
            SearchResult sr;
            sr.pageIndex = pageIndex;
            sr.matchText = data.fullText.substr(pos, query.length());

            size_t ctxStart = (pos > 30) ? pos - 30 : 0;
            size_t ctxEnd = std::min(pos + query.length() + 30, data.fullText.length());
            sr.contextBefore = data.fullText.substr(ctxStart, pos - ctxStart);
            sr.contextAfter = data.fullText.substr(pos + query.length(),
                                                     ctxEnd - pos - query.length());

            // Try to find bounding box from blocks
            for (const auto& block : data.blocks) {
                if (block.text.find(query) != std::wstring::npos) {
                    sr.bounds = block.bounds;
                    break;
                }
            }

            results.push_back(sr);
            pos += query.length();
        }
    }

    // Sort by page index
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.pageIndex < b.pageIndex;
              });

    return results;
}

std::string OCRIndex::computeDocumentHash(const std::wstring& filePath) {
    return utils::computeFileHash(filePath);
}

bool OCRIndex::isFullyIndexed(const std::string& documentHash, int pageCount) const {
    auto docIt = m_index.find(documentHash);
    if (docIt == m_index.end()) return false;
    return static_cast<int>(docIt->second.size()) >= pageCount;
}

bool OCRIndex::saveIndex(const std::string& documentHash) const {
    auto docIt = m_index.find(documentHash);
    if (docIt == m_index.end()) return false;

    std::wstring path = m_cacheDir + L"\\" + utils::utf8ToWide(documentHash) + L".ocr";

    std::ostringstream json;
    json << "{\n  \"pages\": {\n";
    bool first = true;
    for (const auto& [pageIdx, data] : docIt->second) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pageIdx << "\": {"
             << "\"text\": \"" << utils::wideToUtf8(data.fullText) << "\""
             << "}";
    }
    json << "\n  }\n}\n";

    return utils::writeFileText(path, json.str());
}

bool OCRIndex::loadIndex(const std::string& documentHash) {
    std::wstring path = m_cacheDir + L"\\" + utils::utf8ToWide(documentHash) + L".ocr";
    if (!utils::fileExists(path)) return false;
    // TODO: parse JSON and populate index
    return true;
}

void OCRIndex::clearIndex(const std::string& documentHash) {
    m_index.erase(documentHash);
}

void OCRIndex::clearAllIndexes() {
    m_index.clear();
}

size_t OCRIndex::getIndexedPageCount(const std::string& documentHash) const {
    auto docIt = m_index.find(documentHash);
    if (docIt == m_index.end()) return 0;
    return docIt->second.size();
}

} // namespace docvision
