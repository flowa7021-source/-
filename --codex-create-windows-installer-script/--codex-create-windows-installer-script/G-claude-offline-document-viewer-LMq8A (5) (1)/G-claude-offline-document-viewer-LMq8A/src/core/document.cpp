#include "core/document.h"
#include "formats/format_registry.h"
#include "utils/string_utils.h"

namespace docvision {

std::unique_ptr<IDocument> DocumentFactory::createFromFile(const std::wstring& path) {
    DocumentFormat format = detectFormat(path);
    if (format == DocumentFormat::Unknown) {
        return nullptr;
    }
    auto doc = FormatRegistry::instance().createDocument(format);
    if (doc && doc->open(path)) {
        return doc;
    }
    return nullptr;
}

DocumentFormat DocumentFactory::detectFormat(const std::wstring& path) {
    return FormatRegistry::instance().detectFormat(path);
}

bool DocumentFactory::isSupported(const std::wstring& path) {
    return FormatRegistry::instance().isSupported(path);
}

std::vector<std::wstring> DocumentFactory::supportedExtensions() {
    return FormatRegistry::instance().supportedExtensions();
}

} // namespace docvision
