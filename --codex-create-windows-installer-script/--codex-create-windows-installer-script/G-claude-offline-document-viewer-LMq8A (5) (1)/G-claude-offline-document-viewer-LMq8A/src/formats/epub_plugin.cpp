#include "formats/epub_plugin.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"
#include "utils/file_utils.h"

namespace docvision {

EPUBDocument::EPUBDocument() {}
EPUBDocument::~EPUBDocument() { close(); }

bool EPUBDocument::open(const std::wstring& path) {
    m_filePath = path;

    // Extract EPUB (ZIP) to temp directory
    // Parse container.xml -> find OPF -> parse OPF -> build spine + outline
    if (!parseContainer()) {
        LOG_ERROR("EPUB: failed to parse container");
        return false;
    }

    LOG_INFO("EPUB opened: " + utils::wideToUtf8(path) +
             " (" + std::to_string(m_spine.size()) + " spine items)");
    return true;
}

void EPUBDocument::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_spine.clear();
    m_outline.clear();
    m_filePath.clear();
    // Clean up temp extraction directory
    // TODO: remove m_extractDir
}

DocumentMetadata EPUBDocument::getMetadata() const {
    return m_metadata;
}

int EPUBDocument::getPageCount() const {
    return static_cast<int>(m_spine.size());
}

PageSize EPUBDocument::getPageSize(int /*pageIndex*/) const {
    // EPUB doesn't have fixed page sizes — return a reasonable default
    return {612, 792, 0}; // letter size
}

PageBitmap EPUBDocument::renderPage(int pageIndex, double scale, RenderQuality /*quality*/) {
    PageBitmap bitmap;
    // In production: WebView2 renders the HTML content and captures as bitmap
    // For now: return blank placeholder

    PageSize ps = getPageSize(pageIndex);
    int w = static_cast<int>(ps.width * scale);
    int h = static_cast<int>(ps.height * scale);
    bitmap.width = w;
    bitmap.height = h;
    bitmap.stride = w * 4;
    bitmap.scale = scale;
    bitmap.data.resize(h * bitmap.stride, 255);

    return bitmap;
}

PageBitmap EPUBDocument::renderPageRegion(int pageIndex, const Rect& /*region*/,
                                           double scale, RenderQuality quality) {
    return renderPage(pageIndex, scale, quality);
}

std::vector<TextBlock> EPUBDocument::getPageText(int pageIndex) const {
    std::vector<TextBlock> blocks;
    // In production: extract text from HTML content of spine item
    // Would parse the HTML and extract text nodes
    std::wstring text = getPagePlainText(pageIndex);
    if (!text.empty()) {
        TextBlock tb;
        tb.pageIndex = pageIndex;
        tb.text = text;
        blocks.push_back(tb);
    }
    return blocks;
}

std::wstring EPUBDocument::getPagePlainText(int pageIndex) const {
    if (pageIndex < 0 || pageIndex >= static_cast<int>(m_spine.size())) return L"";
    // In production: read HTML file from ZIP, strip tags, return text
    // TODO: implement HTML text extraction
    return L"";
}

std::vector<OutlineItem> EPUBDocument::getOutline() const {
    return m_outline;
}

bool EPUBDocument::hasOutline() const {
    return !m_outline.empty();
}

std::vector<SearchResult> EPUBDocument::search(const std::wstring& query,
                                                bool /*caseSensitive*/,
                                                bool /*wholeWord*/) const {
    std::vector<SearchResult> results;
    for (int i = 0; i < getPageCount(); ++i) {
        std::wstring text = getPagePlainText(i);
        size_t pos = 0;
        while ((pos = text.find(query, pos)) != std::wstring::npos) {
            SearchResult sr;
            sr.pageIndex = i;
            sr.matchText = query;
            results.push_back(sr);
            pos += query.length();
        }
    }
    return results;
}

bool EPUBDocument::parseContainer() {
    // Parse META-INF/container.xml to find rootfile (OPF path)
    // In production: extract from ZIP and parse XML
    // TODO: implement with minizip-ng + a lightweight XML parser

    m_metadata.format = L"EPUB";
    m_metadata.title = utils::getFileName(m_filePath);
    return true;
}

bool EPUBDocument::parseOpf(const std::string& /*opfPath*/) {
    // Parse OPF to build spine, metadata, and manifest
    // TODO: implement
    return true;
}

bool EPUBDocument::parseNavDoc(const std::string& /*navPath*/) {
    // Parse navigation document (EPUB3) or NCX (EPUB2) for TOC
    // TODO: implement
    return true;
}

std::vector<uint8_t> EPUBDocument::readEntryFromZip(const std::string& /*entryPath*/) const {
    // Read a file entry from the EPUB ZIP archive
    // TODO: implement with minizip-ng
    return {};
}

void EPUBDocument::setWebViewHwnd(HWND hwnd) {
    m_webViewHwnd = hwnd;
}

bool EPUBDocument::isWebView2Available() const {
    // Check if WebView2 Runtime is installed
    // In production: use GetAvailableCoreWebView2BrowserVersionString
    return false;
}

} // namespace docvision
