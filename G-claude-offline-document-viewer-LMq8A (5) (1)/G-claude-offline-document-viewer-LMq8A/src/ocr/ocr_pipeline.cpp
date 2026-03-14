#include "ocr/ocr_pipeline.h"
#include "utils/file_utils.h"
#include "diagnostics/logger.h"

namespace docvision {

OCRPipeline::OCRPipeline() {}
OCRPipeline::~OCRPipeline() {}

bool OCRPipeline::initialize(const std::wstring& tessdataPath, const std::wstring& cacheDir) {
    if (!m_index.initialize(cacheDir)) {
        LOG_WARNING("OCR index initialization failed");
    }
    return m_engine.initialize(tessdataPath, "rus");
}

void OCRPipeline::setDocument(IDocument* document, const std::wstring& filePath) {
    m_document = document;
    m_currentDocHash = OCRIndex::computeDocumentHash(filePath);
    m_index.loadIndex(m_currentDocHash);
}

OCRPageResult OCRPipeline::ocrSelectedArea(const PageBitmap& pageBitmap, const Rect& area) {
    PageBitmap preprocessed = m_engine.preprocess(pageBitmap);
    return m_engine.ocrArea(preprocessed, area);
}

OCRPageResult OCRPipeline::ocrCurrentPage(int pageIndex, double renderScale) {
    if (!m_document) return {};

    // Check cache first
    if (m_index.hasPageResult(m_currentDocHash, pageIndex)) {
        return m_index.getPageResult(m_currentDocHash, pageIndex);
    }

    PageBitmap bitmap = m_document->renderPage(pageIndex, renderScale, RenderQuality::HighQuality);
    if (!bitmap.isValid()) return {};

    PageBitmap preprocessed = m_engine.preprocess(bitmap);
    OCRPageResult result = m_engine.ocrPage(preprocessed, pageIndex);

    // Cache result
    if (result.success) {
        m_index.storePageResult(m_currentDocHash, pageIndex, result);
        m_index.saveIndex(m_currentDocHash);
    }

    return result;
}

void OCRPipeline::ocrDocument(OCREngine::ProgressCallback onProgress,
                               OCREngine::CompletionCallback onComplete) {
    if (!m_document) {
        if (onComplete) onComplete(false);
        return;
    }

    m_engine.ocrDocumentAsync(m_document, onProgress,
        [this, onComplete](bool success) {
            if (success) {
                m_index.saveIndex(m_currentDocHash);
            }
            if (onComplete) onComplete(success);
        });
}

void OCRPipeline::cancelOCR() {
    m_engine.cancelDocumentOCR();
}

void OCRPipeline::makeSearchable(SearchableMode mode,
                                  OCREngine::ProgressCallback onProgress,
                                  OCREngine::CompletionCallback onComplete) {
    if (mode == SearchableMode::SidecarOnly) {
        ocrDocument(onProgress, onComplete);
    } else {
        // EmbedInPDF: OCR and then embed text layer into PDF
        ocrDocument(onProgress, [this, onComplete](bool success) {
            if (success && m_document && m_document->getFormat() == DocumentFormat::PDF) {
                // TODO: embed OCR text layer into PDF using MuPDF
                LOG_INFO("Embedding OCR text layer into PDF");
            }
            if (onComplete) onComplete(success);
        });
    }
}

std::vector<SearchResult> OCRPipeline::searchOCR(const std::wstring& query,
                                                   bool caseSensitive) const {
    return m_index.search(m_currentDocHash, query, caseSensitive);
}

bool OCRPipeline::hasOCRIndex() const {
    return m_index.getIndexedPageCount(m_currentDocHash) > 0;
}

bool OCRPipeline::isFullyIndexed() const {
    if (!m_document) return false;
    return m_index.isFullyIndexed(m_currentDocHash, m_document->getPageCount());
}

bool OCRPipeline::setLanguage(const std::string& lang) {
    return m_engine.setLanguage(lang);
}

std::string OCRPipeline::getLanguage() const {
    return m_engine.getLanguage();
}

std::vector<std::string> OCRPipeline::getAvailableLanguages() const {
    return m_engine.getAvailableLanguages();
}

} // namespace docvision
