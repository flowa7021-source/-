#include "formats/djvu_plugin.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"

#ifdef DOCVISION_HAS_DJVULIBRE
#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>
#endif

namespace docvision {

DjVuDocument::DjVuDocument() {}
DjVuDocument::~DjVuDocument() { close(); }

bool DjVuDocument::open(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_engineMutex);
    m_filePath = path;

#ifdef DOCVISION_HAS_DJVULIBRE
    m_context = ddjvu_context_create("DocVision");
    if (!m_context) {
        LOG_ERROR("DjVuLibre: failed to create context");
        return false;
    }

    std::string utf8Path = utils::wideToUtf8(path);
    m_document = ddjvu_document_create_by_filename(
        static_cast<ddjvu_context_t*>(m_context), utf8Path.c_str(), TRUE);

    if (!m_document) {
        LOG_ERROR("DjVuLibre: failed to open document");
        ddjvu_context_release(static_cast<ddjvu_context_t*>(m_context));
        m_context = nullptr;
        return false;
    }

    // Wait for document to be ready
    ddjvu_message_t* msg;
    while (!ddjvu_document_decoding_done(static_cast<ddjvu_document_t*>(m_document))) {
        msg = ddjvu_message_peek(static_cast<ddjvu_context_t*>(m_context));
        if (msg) ddjvu_message_pop(static_cast<ddjvu_context_t*>(m_context));
    }

    LOG_INFO("DjVu opened: " + utf8Path + " (" + std::to_string(getPageCount()) + " pages)");
    return true;
#else
    LOG_WARNING("DjVu support not compiled (DOCVISION_HAS_DJVULIBRE not defined)");
    return false;
#endif
}

bool DjVuDocument::save(const std::wstring& /*path*/) { return false; }

void DjVuDocument::close() {
    std::lock_guard<std::mutex> lock(m_engineMutex);
#ifdef DOCVISION_HAS_DJVULIBRE
    if (m_document) {
        ddjvu_document_release(static_cast<ddjvu_document_t*>(m_document));
        m_document = nullptr;
    }
    if (m_context) {
        ddjvu_context_release(static_cast<ddjvu_context_t*>(m_context));
        m_context = nullptr;
    }
#endif
    m_filePath.clear();
}

DocumentMetadata DjVuDocument::getMetadata() const {
    DocumentMetadata meta;
    meta.format = L"DjVu";
    meta.pageCount = getPageCount();
    meta.title = utils::getFileName(m_filePath);
    return meta;
}

int DjVuDocument::getPageCount() const {
#ifdef DOCVISION_HAS_DJVULIBRE
    if (!m_document) return 0;
    return ddjvu_document_get_pagenum(static_cast<ddjvu_document_t*>(m_document));
#else
    return 0;
#endif
}

PageSize DjVuDocument::getPageSize(int pageIndex) const {
    PageSize size;
#ifdef DOCVISION_HAS_DJVULIBRE
    if (!m_document) return size;
    ddjvu_pageinfo_t info;
    if (ddjvu_document_get_pageinfo(static_cast<ddjvu_document_t*>(m_document),
                                      pageIndex, &info) == DDJVU_JOB_OK) {
        size.width = info.width * 72.0 / info.dpi;
        size.height = info.height * 72.0 / info.dpi;
        size.rotation = info.rotation;
    }
#else
    (void)pageIndex;
    size.width = 612; size.height = 792;
#endif
    return size;
}

PageBitmap DjVuDocument::renderPage(int pageIndex, double scale, RenderQuality quality) {
    std::lock_guard<std::mutex> lock(m_engineMutex);
    PageBitmap bitmap;

#ifdef DOCVISION_HAS_DJVULIBRE
    if (!m_context || !m_document) return bitmap;

    ddjvu_page_t* page = ddjvu_page_create_by_pageno(
        static_cast<ddjvu_document_t*>(m_document), pageIndex);
    if (!page) return bitmap;

    // Wait for page decode
    while (!ddjvu_page_decoding_done(page)) {
        ddjvu_message_t* msg = ddjvu_message_peek(static_cast<ddjvu_context_t*>(m_context));
        if (msg) ddjvu_message_pop(static_cast<ddjvu_context_t*>(m_context));
    }

    int w = static_cast<int>(ddjvu_page_get_width(page) * scale);
    int h = static_cast<int>(ddjvu_page_get_height(page) * scale);

    if (quality == RenderQuality::Draft) {
        w /= 2; h /= 2;
    }

    ddjvu_rect_t pageRect = {0, 0, static_cast<unsigned int>(w), static_cast<unsigned int>(h)};
    ddjvu_rect_t renderRect = pageRect;

    ddjvu_format_t* fmt = ddjvu_format_create(DDJVU_FORMAT_BGR24, 0, nullptr);
    ddjvu_format_set_row_order(fmt, 1);

    int stride = ((w * 3 + 3) / 4) * 4;
    std::vector<uint8_t> rowData(h * stride);

    if (ddjvu_page_render(page, DDJVU_RENDER_COLOR, &pageRect, &renderRect,
                           fmt, stride, reinterpret_cast<char*>(rowData.data()))) {
        // Convert BGR24 to BGRA32
        bitmap.width = w;
        bitmap.height = h;
        bitmap.stride = w * 4;
        bitmap.scale = scale;
        bitmap.data.resize(h * bitmap.stride);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int srcIdx = y * stride + x * 3;
                int dstIdx = y * bitmap.stride + x * 4;
                bitmap.data[dstIdx + 0] = rowData[srcIdx + 0]; // B
                bitmap.data[dstIdx + 1] = rowData[srcIdx + 1]; // G
                bitmap.data[dstIdx + 2] = rowData[srcIdx + 2]; // R
                bitmap.data[dstIdx + 3] = 255;                  // A
            }
        }
    }

    ddjvu_format_release(fmt);
    ddjvu_page_release(page);
#else
    (void)quality;
    PageSize ps = getPageSize(pageIndex);
    int w = static_cast<int>(ps.width * scale);
    int h = static_cast<int>(ps.height * scale);
    bitmap.width = w; bitmap.height = h;
    bitmap.stride = w * 4; bitmap.scale = scale;
    bitmap.data.resize(h * bitmap.stride, 245);
#endif

    return bitmap;
}

PageBitmap DjVuDocument::renderPageRegion(int pageIndex, const Rect& /*region*/,
                                           double scale, RenderQuality quality) {
    return renderPage(pageIndex, scale, quality);
}

std::vector<TextBlock> DjVuDocument::getPageText(int pageIndex) const {
    std::vector<TextBlock> blocks;
#ifdef DOCVISION_HAS_DJVULIBRE
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_document) return blocks;

    miniexp_t expr = ddjvu_document_get_pagetext(
        static_cast<ddjvu_document_t*>(m_document), pageIndex, "word");

    // Parse S-expression text layer
    // DjVuLibre text layer format: (page x0 y0 x1 y1 (line ...) ...)
    // Simplified extraction — walk the expression tree
    if (expr != miniexp_nil && miniexp_consp(expr)) {
        // Recursive text extraction would go here
        // For now, get plain text
        std::wstring text = getPagePlainText(pageIndex);
        if (!text.empty()) {
            TextBlock tb;
            tb.pageIndex = pageIndex;
            tb.text = text;
            blocks.push_back(tb);
        }
    }
#else
    (void)pageIndex;
#endif
    return blocks;
}

std::wstring DjVuDocument::getPagePlainText(int pageIndex) const {
#ifdef DOCVISION_HAS_DJVULIBRE
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_document) return L"";

    miniexp_t expr = ddjvu_document_get_pagetext(
        static_cast<ddjvu_document_t*>(m_document), pageIndex, "word");

    if (expr == miniexp_nil) return L"";

    // Extract all leaf strings from the S-expression
    std::wstring result;
    std::function<void(miniexp_t)> extractText = [&](miniexp_t e) {
        if (miniexp_stringp(e)) {
            const char* str = miniexp_to_str(e);
            if (str) {
                if (!result.empty()) result += L" ";
                result += utils::utf8ToWide(str);
            }
        } else if (miniexp_consp(e)) {
            for (miniexp_t cur = e; cur != miniexp_nil; cur = miniexp_cdr(cur)) {
                extractText(miniexp_car(cur));
            }
        }
    };
    extractText(expr);
    return result;
#else
    (void)pageIndex;
    return L"";
#endif
}

bool DjVuDocument::hasTextLayer(int pageIndex) const {
    return !getPagePlainText(pageIndex).empty();
}

std::vector<OutlineItem> DjVuDocument::getOutline() const {
    std::vector<OutlineItem> outline;
#ifdef DOCVISION_HAS_DJVULIBRE
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_document) return outline;

    miniexp_t expr = ddjvu_document_get_outline(static_cast<ddjvu_document_t*>(m_document));
    // Parse outline S-expression
    // Format: (bookmarks ("title" "#pageN" ...) ...)
    if (expr != miniexp_nil && miniexp_consp(expr)) {
        // Simplified: just return empty for now
        // Full implementation would recursively parse the expression
    }
#endif
    return outline;
}

bool DjVuDocument::hasOutline() const { return !getOutline().empty(); }

std::vector<SearchResult> DjVuDocument::search(const std::wstring& query,
                                                bool /*caseSensitive*/,
                                                bool /*wholeWord*/) const {
    std::vector<SearchResult> results;
    int pageCount = getPageCount();
    for (int i = 0; i < pageCount; ++i) {
        std::wstring text = getPagePlainText(i);
        size_t pos = 0;
        while ((pos = text.find(query, pos)) != std::wstring::npos) {
            SearchResult sr;
            sr.pageIndex = i;
            sr.matchText = query;
            size_t ctxStart = (pos > 20) ? pos - 20 : 0;
            sr.contextBefore = text.substr(ctxStart, pos - ctxStart);
            size_t ctxEnd = std::min(pos + query.length() + 20, text.length());
            sr.contextAfter = text.substr(pos + query.length(), ctxEnd - pos - query.length());
            results.push_back(sr);
            pos += query.length();
        }
    }
    return results;
}

} // namespace docvision
