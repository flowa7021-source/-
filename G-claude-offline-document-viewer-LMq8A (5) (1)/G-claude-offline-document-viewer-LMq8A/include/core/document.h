#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <cstdint>

namespace docvision {

// Forward declarations
struct PageBitmap;
struct TextBlock;
struct OutlineItem;
struct SearchResult;
struct DocumentMetadata;

// Rectangle in page coordinates (points, 72 dpi)
struct Rect {
    double x = 0, y = 0, width = 0, height = 0;

    bool contains(double px, double py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
    bool isEmpty() const { return width <= 0 || height <= 0; }
};

// Page size in points
struct PageSize {
    double width = 0;
    double height = 0;
    int rotation = 0; // 0, 90, 180, 270
};

// Rendered page bitmap
struct PageBitmap {
    std::vector<uint8_t> data;  // BGRA pixel data
    int width = 0;
    int height = 0;
    int stride = 0;
    double scale = 1.0;         // rendering scale factor

    bool isValid() const { return !data.empty() && width > 0 && height > 0; }
};

// Text block with position information
struct TextBlock {
    std::wstring text;
    Rect bounds;
    int pageIndex = 0;
    double fontSize = 0;
    bool isFromOCR = false;
};

// Document outline/TOC entry
struct OutlineItem {
    std::wstring title;
    int pageIndex = -1;
    std::vector<OutlineItem> children;
    bool isOpen = false;
};

// Search result
struct SearchResult {
    int pageIndex = 0;
    Rect bounds;
    std::wstring contextBefore;
    std::wstring matchText;
    std::wstring contextAfter;
};

// Document metadata
struct DocumentMetadata {
    std::wstring title;
    std::wstring author;
    std::wstring subject;
    std::wstring creator;
    std::wstring producer;
    std::wstring creationDate;
    std::wstring modificationDate;
    int pageCount = 0;
    std::wstring format; // "PDF", "DjVu", "CBZ", "EPUB"
};

// Document format type
enum class DocumentFormat {
    Unknown,
    PDF,
    DjVu,
    CBZ,
    EPUB
};

// Render quality hint
enum class RenderQuality {
    Draft,      // fast, lower quality (for scrolling)
    Normal,     // standard quality
    HighQuality // print-quality rendering
};

// Post-processing parameters for scan enhancement
struct PostProcessParams {
    float brightness = 0.0f;    // -1.0 to 1.0
    float contrast = 1.0f;      // 0.0 to 3.0
    float gamma = 1.0f;         // 0.1 to 5.0
    bool invertColors = false;
    bool grayscale = false;

    bool isDefault() const {
        return brightness == 0.0f && contrast == 1.0f &&
               gamma == 1.0f && !invertColors && !grayscale;
    }
    void reset() {
        brightness = 0.0f; contrast = 1.0f; gamma = 1.0f;
        invertColors = false; grayscale = false;
    }
};

// Abstract document interface — implemented by each format plugin
class IDocument {
public:
    virtual ~IDocument() = default;

    // Document lifecycle
    virtual bool open(const std::wstring& path) = 0;
    virtual bool save(const std::wstring& path) = 0;
    virtual bool isModified() const = 0;
    virtual void close() = 0;

    // Metadata
    virtual DocumentMetadata getMetadata() const = 0;
    virtual DocumentFormat getFormat() const = 0;
    virtual std::wstring getFilePath() const = 0;

    // Pages
    virtual int getPageCount() const = 0;
    virtual PageSize getPageSize(int pageIndex) const = 0;

    // Rendering
    virtual PageBitmap renderPage(int pageIndex, double scale,
                                   RenderQuality quality = RenderQuality::Normal) = 0;
    virtual PageBitmap renderPageRegion(int pageIndex, const Rect& region,
                                         double scale,
                                         RenderQuality quality = RenderQuality::Normal) = 0;

    // Text extraction
    virtual std::vector<TextBlock> getPageText(int pageIndex) const = 0;
    virtual std::wstring getPagePlainText(int pageIndex) const = 0;
    virtual bool hasTextLayer(int pageIndex) const = 0;

    // Outline / TOC
    virtual std::vector<OutlineItem> getOutline() const = 0;
    virtual bool hasOutline() const = 0;

    // Search
    virtual std::vector<SearchResult> search(const std::wstring& query,
                                              bool caseSensitive = false,
                                              bool wholeWord = false) const = 0;

    // Page operations
    virtual bool canDeletePages() const = 0;
    virtual bool deletePage(int pageIndex) = 0;
    virtual bool canRotatePages() const = 0;
    virtual bool rotatePage(int pageIndex, int angleDegrees) = 0; // 90, 180, 270
    virtual bool canCropPages() const = 0;
    virtual bool cropPage(int pageIndex, const Rect& cropBox) = 0;

    // Capabilities
    virtual bool supportsAnnotations() const = 0;
    virtual bool supportsTextSelection() const = 0;
    virtual bool supportsSave() const = 0;
    virtual bool supportsPageOperations() const = 0;
};

// Document factory — creates appropriate IDocument based on file extension
class DocumentFactory {
public:
    static std::unique_ptr<IDocument> createFromFile(const std::wstring& path);
    static DocumentFormat detectFormat(const std::wstring& path);
    static bool isSupported(const std::wstring& path);
    static std::vector<std::wstring> supportedExtensions();
};

} // namespace docvision
