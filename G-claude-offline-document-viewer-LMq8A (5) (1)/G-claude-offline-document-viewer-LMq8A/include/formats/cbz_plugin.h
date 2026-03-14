#pragma once

#include "core/document.h"
#include <vector>
#include <mutex>

namespace docvision {

// CBZ document implementation (ZIP archive of images)
class CBZDocument : public IDocument {
public:
    CBZDocument();
    ~CBZDocument() override;

    bool open(const std::wstring& path) override;
    bool save(const std::wstring& path) override;
    bool isModified() const override { return m_modified; }
    void close() override;

    DocumentMetadata getMetadata() const override;
    DocumentFormat getFormat() const override { return DocumentFormat::CBZ; }
    std::wstring getFilePath() const override { return m_filePath; }

    int getPageCount() const override;
    PageSize getPageSize(int pageIndex) const override;

    PageBitmap renderPage(int pageIndex, double scale,
                           RenderQuality quality) override;
    PageBitmap renderPageRegion(int pageIndex, const Rect& region,
                                 double scale, RenderQuality quality) override;

    // CBZ has no native text — always empty (OCR provides text)
    std::vector<TextBlock> getPageText(int pageIndex) const override { return {}; }
    std::wstring getPagePlainText(int pageIndex) const override { return L""; }
    bool hasTextLayer(int pageIndex) const override { return false; }

    std::vector<OutlineItem> getOutline() const override { return {}; }
    bool hasOutline() const override { return false; }

    std::vector<SearchResult> search(const std::wstring& query,
                                      bool caseSensitive,
                                      bool wholeWord) const override { return {}; }

    // CBZ page operations: delete/rotate by modifying archive
    bool canDeletePages() const override { return true; }
    bool deletePage(int pageIndex) override;
    bool canRotatePages() const override { return true; }
    bool rotatePage(int pageIndex, int angleDegrees) override;
    bool canCropPages() const override { return false; }
    bool cropPage(int pageIndex, const Rect& cropBox) override { return false; }

    bool supportsAnnotations() const override { return false; } // sidecar only
    bool supportsTextSelection() const override { return false; }
    bool supportsSave() const override { return true; }
    bool supportsPageOperations() const override { return true; }

private:
    struct PageEntry {
        std::string nameInArchive;
        int width = 0;
        int height = 0;
        int rotation = 0;          // visual rotation applied
        std::vector<uint8_t> data; // cached decoded image data (BGRA)
    };

    bool loadArchive();
    bool decodeImage(int pageIndex);
    void sortPages();

    std::wstring m_filePath;
    bool m_modified = false;
    mutable std::mutex m_mutex;
    std::vector<PageEntry> m_pages;
    void* m_zipHandle = nullptr;
};

} // namespace docvision
