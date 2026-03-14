#include "formats/cbz_plugin.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <cctype>

// In production, would use minizip-ng and stb_image
// Stub implementation for structure

namespace docvision {

CBZDocument::CBZDocument() {}
CBZDocument::~CBZDocument() { close(); }

bool CBZDocument::open(const std::wstring& path) {
    m_filePath = path;
    return loadArchive();
}

bool CBZDocument::save(const std::wstring& /*path*/) {
    // Rebuild ZIP archive with modifications
    // TODO: implement with minizip-ng
    m_modified = false;
    return true;
}

void CBZDocument::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pages.clear();
    m_filePath.clear();
    m_modified = false;
}

DocumentMetadata CBZDocument::getMetadata() const {
    DocumentMetadata meta;
    meta.format = L"CBZ";
    meta.pageCount = getPageCount();
    meta.title = utils::getFileName(m_filePath);
    return meta;
}

int CBZDocument::getPageCount() const {
    return static_cast<int>(m_pages.size());
}

PageSize CBZDocument::getPageSize(int pageIndex) const {
    PageSize size;
    if (pageIndex >= 0 && pageIndex < static_cast<int>(m_pages.size())) {
        size.width = m_pages[pageIndex].width;
        size.height = m_pages[pageIndex].height;
        size.rotation = m_pages[pageIndex].rotation;
    }
    return size;
}

PageBitmap CBZDocument::renderPage(int pageIndex, double scale, RenderQuality /*quality*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PageBitmap bitmap;

    if (pageIndex < 0 || pageIndex >= static_cast<int>(m_pages.size())) return bitmap;

    // Decode image if not cached
    if (m_pages[pageIndex].data.empty()) {
        decodeImage(pageIndex);
    }

    const auto& page = m_pages[pageIndex];
    if (page.data.empty()) return bitmap;

    // Scale the image
    int w = static_cast<int>(page.width * scale);
    int h = static_cast<int>(page.height * scale);

    bitmap.width = w;
    bitmap.height = h;
    bitmap.stride = w * 4;
    bitmap.scale = scale;

    if (std::abs(scale - 1.0) < 0.01) {
        bitmap.data = page.data; // direct copy at 1:1
    } else {
        // Nearest-neighbor scaling (simple)
        bitmap.data.resize(h * bitmap.stride);
        for (int y = 0; y < h; ++y) {
            int srcY = y * page.height / h;
            for (int x = 0; x < w; ++x) {
                int srcX = x * page.width / w;
                int srcIdx = (srcY * page.width + srcX) * 4;
                int dstIdx = (y * w + x) * 4;
                if (srcIdx + 3 < static_cast<int>(page.data.size())) {
                    bitmap.data[dstIdx] = page.data[srcIdx];
                    bitmap.data[dstIdx + 1] = page.data[srcIdx + 1];
                    bitmap.data[dstIdx + 2] = page.data[srcIdx + 2];
                    bitmap.data[dstIdx + 3] = page.data[srcIdx + 3];
                }
            }
        }
    }

    return bitmap;
}

PageBitmap CBZDocument::renderPageRegion(int pageIndex, const Rect& /*region*/,
                                          double scale, RenderQuality quality) {
    return renderPage(pageIndex, scale, quality);
}

bool CBZDocument::deletePage(int pageIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (pageIndex < 0 || pageIndex >= static_cast<int>(m_pages.size())) return false;
    m_pages.erase(m_pages.begin() + pageIndex);
    m_modified = true;
    return true;
}

bool CBZDocument::rotatePage(int pageIndex, int angleDegrees) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (pageIndex < 0 || pageIndex >= static_cast<int>(m_pages.size())) return false;
    m_pages[pageIndex].rotation = (m_pages[pageIndex].rotation + angleDegrees) % 360;
    m_pages[pageIndex].data.clear(); // invalidate cached render
    m_modified = true;
    return true;
}

bool CBZDocument::loadArchive() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // In production: use minizip-ng to enumerate ZIP entries
    // For now: stub that creates placeholder pages
    LOG_INFO("CBZ: loading archive " + utils::wideToUtf8(m_filePath));

    // TODO: actual ZIP reading with minizip-ng
    // Enumerate entries, filter to image files, sort by name
    // For each image: store nameInArchive, defer decoding

    // Stub: assume empty archive, will be populated when libraries are linked
    sortPages();
    return true;
}

bool CBZDocument::decodeImage(int /*pageIndex*/) {
    // In production: extract from ZIP and decode with stb_image
    // TODO: implement with stb_image
    return false;
}

void CBZDocument::sortPages() {
    // Sort pages by filename (natural sort order)
    std::sort(m_pages.begin(), m_pages.end(),
              [](const PageEntry& a, const PageEntry& b) {
                  return a.nameInArchive < b.nameInArchive;
              });
}

} // namespace docvision
