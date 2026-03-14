#include "formats/pdf_plugin.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"

// MuPDF headers (conditionally included)
#ifdef DOCVISION_HAS_MUPDF
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#endif

namespace docvision {

PDFDocument::PDFDocument() {}

PDFDocument::~PDFDocument() {
    close();
}

bool PDFDocument::open(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_engineMutex);
    m_filePath = path;

#ifdef DOCVISION_HAS_MUPDF
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!m_ctx) {
        LOG_ERROR("MuPDF: failed to create context");
        return false;
    }

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    fz_try(ctx) {
        fz_register_document_handlers(ctx);
        std::string utf8Path = utils::wideToUtf8(path);
        m_doc = fz_open_document(ctx, utf8Path.c_str());
    }
    fz_catch(ctx) {
        LOG_ERROR("MuPDF: failed to open document: " + std::string(fz_caught_message(ctx)));
        fz_drop_context(ctx);
        m_ctx = nullptr;
        return false;
    }

    LOG_INFO("PDF opened: " + utils::wideToUtf8(path) +
             " (" + std::to_string(getPageCount()) + " pages)");
    return true;
#else
    LOG_WARNING("PDF support not compiled (DOCVISION_HAS_MUPDF not defined)");
    return false;
#endif
}

bool PDFDocument::save(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_engineMutex);

#ifdef DOCVISION_HAS_MUPDF
    if (!m_ctx || !m_doc) return false;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    pdf_document* doc = pdf_document_from_fz_document(ctx, static_cast<fz_document*>(m_doc));
    if (!doc) return false;

    std::string utf8Path = utils::wideToUtf8(path);
    fz_try(ctx) {
        pdf_save_document(ctx, doc, utf8Path.c_str(), nullptr);
        m_modified = false;
    }
    fz_catch(ctx) {
        LOG_ERROR("MuPDF: failed to save: " + std::string(fz_caught_message(ctx)));
        return false;
    }
    return true;
#else
    (void)path;
    return false;
#endif
}

bool PDFDocument::isModified() const {
    return m_modified;
}

void PDFDocument::close() {
    std::lock_guard<std::mutex> lock(m_engineMutex);

#ifdef DOCVISION_HAS_MUPDF
    if (m_doc) {
        fz_context* ctx = static_cast<fz_context*>(m_ctx);
        fz_drop_document(ctx, static_cast<fz_document*>(m_doc));
        m_doc = nullptr;
    }
    if (m_ctx) {
        fz_drop_context(static_cast<fz_context*>(m_ctx));
        m_ctx = nullptr;
    }
#endif
    m_filePath.clear();
    m_modified = false;
}

DocumentMetadata PDFDocument::getMetadata() const {
    DocumentMetadata meta;
    meta.format = L"PDF";

#ifdef DOCVISION_HAS_MUPDF
    if (!m_ctx || !m_doc) return meta;
    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    fz_document* doc = static_cast<fz_document*>(m_doc);

    meta.pageCount = fz_count_pages(ctx, doc);

    auto getInfo = [&](const char* key) -> std::wstring {
        char buf[256] = {};
        fz_lookup_metadata(ctx, doc, key, buf, sizeof(buf));
        return utils::utf8ToWide(buf);
    };

    meta.title = getInfo(FZ_META_INFO_TITLE);
    meta.author = getInfo(FZ_META_INFO_AUTHOR);
    meta.subject = getInfo("info:Subject");
    meta.creator = getInfo("info:Creator");
    meta.producer = getInfo("info:Producer");
#endif
    return meta;
}

int PDFDocument::getPageCount() const {
#ifdef DOCVISION_HAS_MUPDF
    if (!m_ctx || !m_doc) return 0;
    return fz_count_pages(static_cast<fz_context*>(m_ctx),
                           static_cast<fz_document*>(m_doc));
#else
    return 0;
#endif
}

PageSize PDFDocument::getPageSize(int pageIndex) const {
    PageSize size;
#ifdef DOCVISION_HAS_MUPDF
    if (!m_ctx || !m_doc) return size;
    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    fz_document* doc = static_cast<fz_document*>(m_doc);

    fz_page* page = fz_load_page(ctx, doc, pageIndex);
    fz_rect bounds = fz_bound_page(ctx, page);
    size.width = bounds.x1 - bounds.x0;
    size.height = bounds.y1 - bounds.y0;
    fz_drop_page(ctx, page);
#else
    (void)pageIndex;
    size.width = 612; size.height = 792; // default letter size
#endif
    return size;
}

PageBitmap PDFDocument::renderPage(int pageIndex, double scale, RenderQuality quality) {
    std::lock_guard<std::mutex> lock(m_engineMutex);
    PageBitmap bitmap;

#ifdef DOCVISION_HAS_MUPDF
    if (!m_ctx || !m_doc) return bitmap;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    fz_document* doc = static_cast<fz_document*>(m_doc);

    fz_try(ctx) {
        fz_page* page = fz_load_page(ctx, doc, pageIndex);
        fz_rect bounds = fz_bound_page(ctx, page);

        float zoom = static_cast<float>(scale);
        if (quality == RenderQuality::Draft) zoom *= 0.5f;

        fz_matrix ctm = fz_scale(zoom, zoom);
        fz_irect ibox = fz_round_rect(fz_transform_rect(bounds, ctm));

        int w = ibox.x1 - ibox.x0;
        int h = ibox.y1 - ibox.y0;

        fz_pixmap* pix = fz_new_pixmap_with_bbox(ctx, fz_device_bgr(ctx), ibox, nullptr, 1);
        fz_clear_pixmap_with_value(ctx, pix, 255);

        fz_device* dev = fz_new_draw_device(ctx, ctm, pix);
        fz_run_page(ctx, page, dev, fz_identity, nullptr);
        fz_close_device(ctx, dev);
        fz_drop_device(ctx, dev);

        bitmap.width = w;
        bitmap.height = h;
        bitmap.stride = pix->stride;
        bitmap.scale = scale;
        bitmap.data.assign(pix->samples, pix->samples + h * pix->stride);

        fz_drop_pixmap(ctx, pix);
        fz_drop_page(ctx, page);
    }
    fz_catch(ctx) {
        LOG_ERROR("MuPDF render error page " + std::to_string(pageIndex) +
                  ": " + fz_caught_message(ctx));
    }
#else
    // Stub: create blank page bitmap
    (void)quality;
    PageSize ps = getPageSize(pageIndex);
    int w = static_cast<int>(ps.width * scale);
    int h = static_cast<int>(ps.height * scale);
    bitmap.width = w;
    bitmap.height = h;
    bitmap.stride = w * 4;
    bitmap.scale = scale;
    bitmap.data.resize(h * bitmap.stride, 255); // white
#endif

    return bitmap;
}

PageBitmap PDFDocument::renderPageRegion(int pageIndex, const Rect& region,
                                          double scale, RenderQuality quality) {
    // For simplicity, render full page and extract region
    auto fullPage = renderPage(pageIndex, scale, quality);
    // TODO: optimize to render only the region
    return fullPage;
}

std::vector<TextBlock> PDFDocument::getPageText(int pageIndex) const {
    std::vector<TextBlock> blocks;
#ifdef DOCVISION_HAS_MUPDF
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_ctx || !m_doc) return blocks;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    fz_document* doc = static_cast<fz_document*>(m_doc);

    fz_try(ctx) {
        fz_page* page = fz_load_page(ctx, doc, pageIndex);
        fz_stext_page* tp = fz_new_stext_page_from_page(ctx, page, nullptr);

        for (fz_stext_block* block = tp->first_block; block; block = block->next) {
            if (block->type != FZ_STEXT_BLOCK_TEXT) continue;
            for (fz_stext_line* line = block->u.t.first_line; line; line = line->next) {
                TextBlock tb;
                tb.pageIndex = pageIndex;
                tb.bounds = {line->bbox.x0, line->bbox.y0,
                             line->bbox.x1 - line->bbox.x0,
                             line->bbox.y1 - line->bbox.y0};
                for (fz_stext_char* ch = line->first_char; ch; ch = ch->next) {
                    tb.text += static_cast<wchar_t>(ch->c);
                }
                if (!tb.text.empty()) blocks.push_back(std::move(tb));
            }
        }

        fz_drop_stext_page(ctx, tp);
        fz_drop_page(ctx, page);
    }
    fz_catch(ctx) {
        LOG_ERROR("MuPDF text extraction error: " + std::string(fz_caught_message(ctx)));
    }
#else
    (void)pageIndex;
#endif
    return blocks;
}

std::wstring PDFDocument::getPagePlainText(int pageIndex) const {
    auto blocks = getPageText(pageIndex);
    std::wstring result;
    for (const auto& b : blocks) {
        if (!result.empty()) result += L"\n";
        result += b.text;
    }
    return result;
}

bool PDFDocument::hasTextLayer(int pageIndex) const {
    return !getPageText(pageIndex).empty();
}

std::vector<OutlineItem> PDFDocument::getOutline() const {
    std::vector<OutlineItem> outline;
#ifdef DOCVISION_HAS_MUPDF
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_ctx || !m_doc) return outline;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    fz_document* doc = static_cast<fz_document*>(m_doc);

    fz_outline* root = fz_load_outline(ctx, doc);
    std::function<void(fz_outline*, std::vector<OutlineItem>&)> walkOutline;
    walkOutline = [&](fz_outline* node, std::vector<OutlineItem>& items) {
        for (; node; node = node->next) {
            OutlineItem item;
            item.title = node->title ? utils::utf8ToWide(node->title) : L"";
            item.pageIndex = fz_page_number_from_location(ctx, doc, node->page);
            item.isOpen = node->is_open;
            if (node->down) {
                walkOutline(node->down, item.children);
            }
            items.push_back(std::move(item));
        }
    };
    walkOutline(root, outline);
    fz_drop_outline(ctx, root);
#endif
    return outline;
}

bool PDFDocument::hasOutline() const {
    return !getOutline().empty();
}

std::vector<SearchResult> PDFDocument::search(const std::wstring& query,
                                               bool caseSensitive,
                                               bool wholeWord) const {
    std::vector<SearchResult> results;
    (void)caseSensitive;
    (void)wholeWord;

    int pageCount = getPageCount();
    for (int i = 0; i < pageCount; ++i) {
        std::wstring text = getPagePlainText(i);
        size_t pos = 0;
        while ((pos = text.find(query, pos)) != std::wstring::npos) {
            SearchResult sr;
            sr.pageIndex = i;
            sr.matchText = query;

            // Context
            size_t ctxStart = (pos > 20) ? pos - 20 : 0;
            size_t ctxEnd = std::min(pos + query.length() + 20, text.length());
            sr.contextBefore = text.substr(ctxStart, pos - ctxStart);
            sr.contextAfter = text.substr(pos + query.length(), ctxEnd - pos - query.length());

            results.push_back(sr);
            pos += query.length();
        }
    }
    return results;
}

bool PDFDocument::deletePage(int pageIndex) {
#ifdef DOCVISION_HAS_MUPDF
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_ctx || !m_doc) return false;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    pdf_document* doc = pdf_document_from_fz_document(ctx, static_cast<fz_document*>(m_doc));
    if (!doc) return false;

    fz_try(ctx) {
        pdf_delete_page(ctx, doc, pageIndex);
        m_modified = true;
    }
    fz_catch(ctx) {
        LOG_ERROR("Failed to delete page: " + std::string(fz_caught_message(ctx)));
        return false;
    }
    return true;
#else
    (void)pageIndex;
    return false;
#endif
}

bool PDFDocument::rotatePage(int pageIndex, int angleDegrees) {
#ifdef DOCVISION_HAS_MUPDF
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_ctx || !m_doc) return false;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    pdf_document* doc = pdf_document_from_fz_document(ctx, static_cast<fz_document*>(m_doc));
    if (!doc) return false;

    fz_try(ctx) {
        pdf_page* page = pdf_load_page(ctx, doc, pageIndex);
        int current = pdf_to_int(ctx, pdf_dict_get_inheritable(ctx, page->obj, PDF_NAME(Rotate)));
        int newRotation = (current + angleDegrees) % 360;
        pdf_dict_put_int(ctx, page->obj, PDF_NAME(Rotate), newRotation);
        fz_drop_page(ctx, &page->super);
        m_modified = true;
    }
    fz_catch(ctx) {
        LOG_ERROR("Failed to rotate page: " + std::string(fz_caught_message(ctx)));
        return false;
    }
    return true;
#else
    (void)pageIndex; (void)angleDegrees;
    return false;
#endif
}

bool PDFDocument::cropPage(int pageIndex, const Rect& cropBox) {
#ifdef DOCVISION_HAS_MUPDF
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_ctx || !m_doc) return false;

    fz_context* ctx = static_cast<fz_context*>(m_ctx);
    pdf_document* doc = pdf_document_from_fz_document(ctx, static_cast<fz_document*>(m_doc));
    if (!doc) return false;

    fz_try(ctx) {
        pdf_page* page = pdf_load_page(ctx, doc, pageIndex);
        fz_rect crop = {static_cast<float>(cropBox.x),
                        static_cast<float>(cropBox.y),
                        static_cast<float>(cropBox.x + cropBox.width),
                        static_cast<float>(cropBox.y + cropBox.height)};
        pdf_obj* rect = pdf_new_rect(ctx, doc, crop);
        pdf_dict_put(ctx, page->obj, PDF_NAME(CropBox), rect);
        fz_drop_page(ctx, &page->super);
        m_modified = true;
    }
    fz_catch(ctx) {
        LOG_ERROR("Failed to crop page: " + std::string(fz_caught_message(ctx)));
        return false;
    }
    return true;
#else
    (void)pageIndex; (void)cropBox;
    return false;
#endif
}

} // namespace docvision
