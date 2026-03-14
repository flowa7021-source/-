#pragma once

#include "core/document.h"
#include <mutex>

namespace docvision {

// DjVu document implementation using DjVuLibre
class DjVuDocument : public IDocument {
public:
    DjVuDocument();
    ~DjVuDocument() override;

    bool open(const std::wstring& path) override;
    bool save(const std::wstring& path) override;
    bool isModified() const override { return false; } // DjVu: read-only
    void close() override;

    DocumentMetadata getMetadata() const override;
    DocumentFormat getFormat() const override { return DocumentFormat::DjVu; }
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

    // DjVu: page operations are view-only (rotation is visual only)
    bool canDeletePages() const override { return false; }
    bool deletePage(int pageIndex) override { return false; }
    bool canRotatePages() const override { return false; }
    bool rotatePage(int pageIndex, int angleDegrees) override { return false; }
    bool canCropPages() const override { return false; }
    bool cropPage(int pageIndex, const Rect& cropBox) override { return false; }

    bool supportsAnnotations() const override { return false; } // sidecar only
    bool supportsTextSelection() const override { return true; }
    bool supportsSave() const override { return false; }
    bool supportsPageOperations() const override { return false; }

private:
    std::wstring m_filePath;
    mutable std::mutex m_engineMutex;

#ifdef DOCVISION_HAS_DJVULIBRE
    void* m_context = nullptr;   // ddjvu_context_t*
    void* m_document = nullptr;  // ddjvu_document_t*
#endif
};

} // namespace docvision
