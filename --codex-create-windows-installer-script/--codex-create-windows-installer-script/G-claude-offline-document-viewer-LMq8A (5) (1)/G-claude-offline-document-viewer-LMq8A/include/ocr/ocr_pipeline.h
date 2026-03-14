#pragma once

#include "ocr/ocr_engine.h"
#include "ocr/ocr_index.h"
#include "core/document.h"

namespace docvision {

// OCR pipeline — coordinates OCR engine, index, and document integration
class OCRPipeline {
public:
    OCRPipeline();
    ~OCRPipeline();

    bool initialize(const std::wstring& tessdataPath, const std::wstring& cacheDir);

    // Set current document
    void setDocument(IDocument* document, const std::wstring& filePath);

    // OCR operations
    OCRPageResult ocrSelectedArea(const PageBitmap& pageBitmap, const Rect& area);
    OCRPageResult ocrCurrentPage(int pageIndex, double renderScale = 2.0);
    void ocrDocument(OCREngine::ProgressCallback onProgress,
                      OCREngine::CompletionCallback onComplete);
    void cancelOCR();

    // "Make searchable" mode
    enum class SearchableMode {
        SidecarOnly,        // store OCR in sidecar index (don't modify file)
        EmbedInPDF          // embed text layer into PDF
    };
    void makeSearchable(SearchableMode mode,
                         OCREngine::ProgressCallback onProgress,
                         OCREngine::CompletionCallback onComplete);

    // Search through OCR index
    std::vector<SearchResult> searchOCR(const std::wstring& query,
                                         bool caseSensitive = false) const;

    // Check if current document has OCR index
    bool hasOCRIndex() const;
    bool isFullyIndexed() const;

    // Language
    bool setLanguage(const std::string& lang);
    std::string getLanguage() const;
    std::vector<std::string> getAvailableLanguages() const;

    // Components access
    OCREngine* getEngine() { return &m_engine; }
    OCRIndex* getIndex() { return &m_index; }

private:
    OCREngine m_engine;
    OCRIndex m_index;
    IDocument* m_document = nullptr;
    std::string m_currentDocHash;
};

} // namespace docvision
