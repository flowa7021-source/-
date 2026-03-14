#pragma once

#include "core/document.h"
#include <mutex>

namespace docvision {

// PDF document implementation using MuPDF + QPDF
class PDFDocument : public IDocument {
public:
    PDFDocument();
    ~PDFDocument() override;

    // IDocument interface
    bool open(const std::wstring& path) override;
    bool save(const std::wstring& path) override;
    bool isModified() const override;
    void close() override;

    DocumentMetadata getMetadata() const override;
    DocumentFormat getFormat() const override { return DocumentFormat::PDF; }
    std::wstring getFilePath() const override { return m_filePath; }

    int getPageCount() const override;
    PageSize getPageSize(int pageIndex) const override;

    PageBitmap renderPage(int pageIndex, double scale,
                           RenderQuality quality) override;
    PageBitmap renderPageRegion(int pageIndex, const Rect& region,
                                 double scale, RenderQuality quality) override;

    std::vector<TextBlock> getPageText(int pageIndex) const override;
    std::wstring getPagePlainText(int pageIndex) const override;
    bool hasTextLayer(int pageIndex) const override;

    std::vector<OutlineItem> getOutline() const override;
    bool hasOutline() const override;

    std::vector<SearchResult> search(const std::wstring& query,
                                      bool caseSensitive,
                                      bool wholeWord) const override;

    bool canDeletePages() const override { return true; }
    bool deletePage(int pageIndex) override;
    bool canRotatePages() const override { return true; }
    bool rotatePage(int pageIndex, int angleDegrees) override;
    bool canCropPages() const override { return true; }
    bool cropPage(int pageIndex, const Rect& cropBox) override;

    bool supportsAnnotations() const override { return true; }
    bool supportsTextSelection() const override { return true; }
    bool supportsSave() const override { return true; }
    bool supportsPageOperations() const override { return true; }

    // PDF-specific: annotation support
    // (Exposed via AnnotationManager, but backed by MuPDF pdf_annot)

private:
    std::wstring m_filePath;
    bool m_modified = false;
    mutable std::mutex m_engineMutex; // serialize MuPDF calls

#ifdef DOCVISION_HAS_MUPDF
    void* m_ctx = nullptr;       // fz_context*
    void* m_doc = nullptr;       // fz_document*
#endif
};

} // namespace docvision
