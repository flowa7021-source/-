#include "formats/format_registry.h"
#include "formats/pdf_plugin.h"
#include "formats/djvu_plugin.h"
#include "formats/cbz_plugin.h"
#include "formats/epub_plugin.h"
#include "utils/string_utils.h"
#include <algorithm>

namespace docvision {

FormatRegistry& FormatRegistry::instance() {
    static FormatRegistry s_instance;

    // Auto-register formats on first access
    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        s_instance.registerFormat(DocumentFormat::PDF,
            {L".pdf"},
            []() { return std::make_unique<PDFDocument>(); });

        s_instance.registerFormat(DocumentFormat::DjVu,
            {L".djvu", L".djv"},
            []() { return std::make_unique<DjVuDocument>(); });

        s_instance.registerFormat(DocumentFormat::CBZ,
            {L".cbz"},
            []() { return std::make_unique<CBZDocument>(); });

        s_instance.registerFormat(DocumentFormat::EPUB,
            {L".epub"},
            []() { return std::make_unique<EPUBDocument>(); });
    }

    return s_instance;
}

void FormatRegistry::registerFormat(DocumentFormat format,
                                     const std::vector<std::wstring>& extensions,
                                     DocumentCreator creator) {
    m_formats.push_back({format, extensions, std::move(creator)});
}

std::unique_ptr<IDocument> FormatRegistry::createDocument(DocumentFormat format) const {
    for (const auto& entry : m_formats) {
        if (entry.format == format) {
            return entry.creator();
        }
    }
    return nullptr;
}

DocumentFormat FormatRegistry::detectFormat(const std::wstring& path) const {
    std::wstring ext = utils::getFileExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    for (const auto& entry : m_formats) {
        for (const auto& supportedExt : entry.extensions) {
            if (ext == supportedExt) {
                return entry.format;
            }
        }
    }
    return DocumentFormat::Unknown;
}

bool FormatRegistry::isSupported(const std::wstring& path) const {
    return detectFormat(path) != DocumentFormat::Unknown;
}

std::vector<std::wstring> FormatRegistry::supportedExtensions() const {
    std::vector<std::wstring> result;
    for (const auto& entry : m_formats) {
        for (const auto& ext : entry.extensions) {
            result.push_back(ext);
        }
    }
    return result;
}

} // namespace docvision
