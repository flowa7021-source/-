#pragma once

#include "core/document.h"
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace docvision {

// OCR mode
enum class OCRMode {
    Area,           // selected region only
    Page,           // single page
    Document        // entire document (background)
};

// OCR result for a single region
struct OCRResult {
    std::wstring text;
    Rect bounds;
    float confidence = 0.0f;
    int pageIndex = 0;
};

// OCR page result
struct OCRPageResult {
    int pageIndex = 0;
    std::vector<OCRResult> blocks;
    std::wstring fullText;
    bool success = false;
    std::wstring errorMessage;
};

// OCR progress info
struct OCRProgress {
    int totalPages = 0;
    int completedPages = 0;
    int currentPage = 0;
    double fraction() const {
        return totalPages > 0 ? static_cast<double>(completedPages) / totalPages : 0.0;
    }
};

// OCR engine — Tesseract wrapper with preprocessing pipeline
class OCREngine {
public:
    OCREngine();
    ~OCREngine();

    // Initialize with tessdata path and language
    bool initialize(const std::wstring& tessdataPath, const std::string& language = "rus");
    bool isInitialized() const { return m_initialized; }
    void shutdown();

    // Set language (reinitializes if needed)
    bool setLanguage(const std::string& language);
    std::string getLanguage() const { return m_language; }
    std::vector<std::string> getAvailableLanguages() const;

    // OCR operations
    OCRPageResult ocrArea(const PageBitmap& bitmap, const Rect& area);
    OCRPageResult ocrPage(const PageBitmap& bitmap, int pageIndex);

    // Document OCR (async with progress)
    using ProgressCallback = std::function<void(const OCRProgress&)>;
    using CompletionCallback = std::function<void(bool success)>;
    void ocrDocumentAsync(IDocument* document,
                           ProgressCallback onProgress,
                           CompletionCallback onComplete);
    void cancelDocumentOCR();
    bool isDocumentOCRRunning() const { return m_ocrRunning; }

    // Preprocessing
    struct PreprocessOptions {
        bool deskew = true;
        bool denoise = true;
        bool binarize = true;
        bool autoOrientationDetect = true;
    };
    void setPreprocessOptions(const PreprocessOptions& options);
    PageBitmap preprocess(const PageBitmap& input);

    // Auto-detect page orientation (returns degrees: 0, 90, 180, 270)
    int detectOrientation(const PageBitmap& bitmap);

private:
    bool m_initialized = false;
    std::string m_language;
    std::wstring m_tessdataPath;
    PreprocessOptions m_preprocessOptions;
    std::atomic<bool> m_ocrRunning{false};
    std::atomic<bool> m_ocrCancelled{false};

#ifdef DOCVISION_HAS_TESSERACT
    void* m_tessApi = nullptr;  // tesseract::TessBaseAPI*
#endif
};

} // namespace docvision
