#include "ocr/ocr_engine.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"
#include <thread>
#include <filesystem>

#ifdef DOCVISION_HAS_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

namespace docvision {

OCREngine::OCREngine() {}

OCREngine::~OCREngine() {
    shutdown();
}

bool OCREngine::initialize(const std::wstring& tessdataPath, const std::string& language) {
    m_tessdataPath = tessdataPath;
    m_language = language;

#ifdef DOCVISION_HAS_TESSERACT
    auto* api = new tesseract::TessBaseAPI();
    std::string utf8Path = utils::wideToUtf8(tessdataPath);

    if (api->Init(utf8Path.c_str(), language.c_str(), tesseract::OEM_LSTM_ONLY)) {
        LOG_ERROR("Tesseract: failed to initialize with language " + language);
        delete api;
        return false;
    }

    api->SetPageSegMode(tesseract::PSM_AUTO);
    m_tessApi = api;
    m_initialized = true;
    LOG_INFO("Tesseract OCR initialized: language=" + language);
    return true;
#else
    LOG_WARNING("OCR support not compiled (DOCVISION_HAS_TESSERACT not defined)");
    return false;
#endif
}

void OCREngine::shutdown() {
#ifdef DOCVISION_HAS_TESSERACT
    if (m_tessApi) {
        auto* api = static_cast<tesseract::TessBaseAPI*>(m_tessApi);
        api->End();
        delete api;
        m_tessApi = nullptr;
    }
#endif
    m_initialized = false;
    m_ocrCancelled = true;
}

bool OCREngine::setLanguage(const std::string& language) {
    if (language == m_language && m_initialized) return true;
    shutdown();
    return initialize(m_tessdataPath, language);
}

std::vector<std::string> OCREngine::getAvailableLanguages() const {
    std::vector<std::string> languages;
    // Scan tessdata directory for .traineddata files
    try {
        std::string dir = utils::wideToUtf8(m_tessdataPath);
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            std::string name = entry.path().stem().string();
            if (entry.path().extension() == ".traineddata") {
                languages.push_back(name);
            }
        }
    } catch (...) {
        // tessdata directory might not exist yet
    }
    if (languages.empty()) {
        languages.push_back("rus");
        languages.push_back("eng");
    }
    return languages;
}

OCRPageResult OCREngine::ocrArea(const PageBitmap& bitmap, const Rect& area) {
    OCRPageResult result;
    result.success = false;

#ifdef DOCVISION_HAS_TESSERACT
    if (!m_initialized || !m_tessApi) {
        result.errorMessage = L"OCR engine not initialized";
        return result;
    }

    auto* api = static_cast<tesseract::TessBaseAPI*>(m_tessApi);

    // Create Pix from bitmap data (BGRA -> RGB)
    int x = static_cast<int>(area.x);
    int y = static_cast<int>(area.y);
    int w = static_cast<int>(area.width);
    int h = static_cast<int>(area.height);

    // Clamp to bitmap bounds
    x = std::max(0, std::min(x, bitmap.width - 1));
    y = std::max(0, std::min(y, bitmap.height - 1));
    w = std::min(w, bitmap.width - x);
    h = std::min(h, bitmap.height - y);

    if (w <= 0 || h <= 0) return result;

    Pix* pix = pixCreate(w, h, 32);
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            int srcIdx = ((y + py) * bitmap.stride) + ((x + px) * 4);
            uint8_t b = bitmap.data[srcIdx];
            uint8_t g = bitmap.data[srcIdx + 1];
            uint8_t r = bitmap.data[srcIdx + 2];
            pixSetRGBPixel(pix, px, py, r, g, b);
        }
    }

    api->SetImage(pix);
    char* text = api->GetUTF8Text();
    if (text) {
        result.fullText = utils::utf8ToWide(text);
        result.success = true;

        // Get word-level results with bounding boxes
        tesseract::ResultIterator* ri = api->GetIterator();
        if (ri) {
            do {
                const char* word = ri->GetUTF8Text(tesseract::RIL_WORD);
                if (word) {
                    OCRResult ocrBlock;
                    ocrBlock.text = utils::utf8ToWide(word);
                    ocrBlock.confidence = ri->Confidence(tesseract::RIL_WORD);

                    int bx, by, bw, bh;
                    ri->BoundingBox(tesseract::RIL_WORD, &bx, &by, &bw, &bh);
                    ocrBlock.bounds = {static_cast<double>(bx + x),
                                       static_cast<double>(by + y),
                                       static_cast<double>(bw - bx),
                                       static_cast<double>(bh - by)};

                    result.blocks.push_back(ocrBlock);
                    delete[] word;
                }
            } while (ri->Next(tesseract::RIL_WORD));
        }

        delete[] text;
    }

    pixDestroy(&pix);
#else
    (void)bitmap; (void)area;
    result.errorMessage = L"OCR not available";
#endif

    return result;
}

OCRPageResult OCREngine::ocrPage(const PageBitmap& bitmap, int pageIndex) {
    Rect fullPage = {0, 0, static_cast<double>(bitmap.width), static_cast<double>(bitmap.height)};
    OCRPageResult result = ocrArea(bitmap, fullPage);
    result.pageIndex = pageIndex;
    return result;
}

void OCREngine::ocrDocumentAsync(IDocument* document,
                                  ProgressCallback onProgress,
                                  CompletionCallback onComplete) {
    m_ocrRunning = true;
    m_ocrCancelled = false;

    std::thread([this, document, onProgress, onComplete]() {
        int pageCount = document->getPageCount();
        OCRProgress progress;
        progress.totalPages = pageCount;

        bool success = true;
        for (int i = 0; i < pageCount && !m_ocrCancelled; ++i) {
            progress.currentPage = i;

            // Render page at 2x for better OCR accuracy
            PageBitmap bitmap = document->renderPage(i, 2.0, RenderQuality::HighQuality);
            if (bitmap.isValid()) {
                ocrPage(bitmap, i);
            }

            progress.completedPages = i + 1;
            if (onProgress) onProgress(progress);
        }

        m_ocrRunning = false;
        if (onComplete) onComplete(success && !m_ocrCancelled);
    }).detach();
}

void OCREngine::cancelDocumentOCR() {
    m_ocrCancelled = true;
}

void OCREngine::setPreprocessOptions(const PreprocessOptions& options) {
    m_preprocessOptions = options;
}

PageBitmap OCREngine::preprocess(const PageBitmap& input) {
    PageBitmap result = input;

    // Basic binarization for better OCR
    if (m_preprocessOptions.binarize) {
        for (size_t i = 0; i < result.data.size(); i += 4) {
            float gray = 0.299f * result.data[i+2] + 0.587f * result.data[i+1] + 0.114f * result.data[i];
            uint8_t bw = (gray > 128) ? 255 : 0;
            result.data[i] = result.data[i+1] = result.data[i+2] = bw;
        }
    }

    return result;
}

int OCREngine::detectOrientation(const PageBitmap& /*bitmap*/) {
#ifdef DOCVISION_HAS_TESSERACT
    // Use Tesseract's orientation detection
    // api->DetectOrientationScript(...)
#endif
    return 0; // default: no rotation needed
}

} // namespace docvision
