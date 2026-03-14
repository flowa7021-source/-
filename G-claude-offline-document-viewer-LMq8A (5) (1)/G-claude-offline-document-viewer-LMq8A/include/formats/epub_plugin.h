#pragma once

#include "core/document.h"
#include <vector>
#include <mutex>

namespace docvision {

// EPUB document implementation using WebView2 for HTML rendering
class EPUBDocument : public IDocument {
public:
    EPUBDocument();
    ~EPUBDocument() override;

    bool open(const std::wstring& path) override;
    bool save(const std::wstring& path) override { return false; } // read-only
    bool isModified() const override { return false; }
    void close() override;

    DocumentMetadata getMetadata() const override;
    DocumentFormat getFormat() const override { return DocumentFormat::EPUB; }
    std::wstring getFilePath() const override { return m_filePath; }

    int getPageCount() const override;          // "pages" = spine items / chapters
    PageSize getPageSize(int pageIndex) const override;

    PageBitmap renderPage(int pageIndex, double scale,
                           RenderQuality quality) override;
    PageBitmap renderPageRegion(int pageIndex, const Rect& region,
                                 double scale, RenderQuality quality) override;

    std::vector<TextBlock> getPageText(int pageIndex) const override;
    std::wstring getPagePlainText(int pageIndex) const override;
    bool hasTextLayer(int pageIndex) const override { return true; } // EPUB is always text

    std::vector<OutlineItem> getOutline() const override;
    bool hasOutline() const override;

    std::vector<SearchResult> search(const std::wstring& query,
                                      bool caseSensitive,
                                      bool wholeWord) const override;

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

    // EPUB-specific
    struct SpineItem {
        std::string id;
        std::string href;
        std::string mediaType;
        std::wstring title;
    };
    std::vector<SpineItem> getSpine() const { return m_spine; }

    // WebView2 integration
    void setWebViewHwnd(HWND hwnd);
    bool isWebView2Available() const;

private:
    bool parseContainer();
    bool parseOpf(const std::string& opfPath);
    bool parseNavDoc(const std::string& navPath);
    std::vector<uint8_t> readEntryFromZip(const std::string& entryPath) const;

    std::wstring m_filePath;
    std::wstring m_extractDir;   // temp directory for extracted content
    mutable std::mutex m_mutex;

    // EPUB structure
    DocumentMetadata m_metadata;
    std::vector<SpineItem> m_spine;
    std::vector<OutlineItem> m_outline;

    // WebView2
    HWND m_webViewHwnd = nullptr;
    void* m_webViewController = nullptr; // ICoreWebView2Controller*
    void* m_webView = nullptr;           // ICoreWebView2*

    void* m_zipHandle = nullptr;
};

} // namespace docvision
