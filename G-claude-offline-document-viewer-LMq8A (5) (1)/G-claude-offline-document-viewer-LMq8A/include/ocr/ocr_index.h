#pragma once

#include "core/document.h"
#include "ocr/ocr_engine.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace docvision {

// OCR index — stores OCR results keyed by document content hash
class OCRIndex {
public:
    OCRIndex();
    ~OCRIndex();

    // Initialize with cache directory
    bool initialize(const std::wstring& cacheDir);

    // Store / retrieve OCR results
    void storePageResult(const std::string& documentHash, int pageIndex,
                          const OCRPageResult& result);
    bool hasPageResult(const std::string& documentHash, int pageIndex) const;
    OCRPageResult getPageResult(const std::string& documentHash, int pageIndex) const;

    // Full-text search over OCR index
    std::vector<SearchResult> search(const std::string& documentHash,
                                      const std::wstring& query,
                                      bool caseSensitive = false) const;

    // Document hash computation
    static std::string computeDocumentHash(const std::wstring& filePath);

    // Check if document is fully indexed
    bool isFullyIndexed(const std::string& documentHash, int pageCount) const;

    // Persistence
    bool saveIndex(const std::string& documentHash) const;
    bool loadIndex(const std::string& documentHash);
    void clearIndex(const std::string& documentHash);
    void clearAllIndexes();

    // Statistics
    size_t getIndexedPageCount(const std::string& documentHash) const;

private:
    std::wstring m_cacheDir;

    struct PageOCRData {
        std::vector<OCRResult> blocks;
        std::wstring fullText;
    };

    // documentHash -> (pageIndex -> OCR data)
    std::unordered_map<std::string,
        std::unordered_map<int, PageOCRData>> m_index;
};

} // namespace docvision
