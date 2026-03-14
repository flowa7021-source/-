#pragma once

#include "core/document.h"
#include <unordered_map>
#include <functional>

namespace docvision {

// Format plugin registration
class FormatRegistry {
public:
    static FormatRegistry& instance();

    using DocumentCreator = std::function<std::unique_ptr<IDocument>()>;

    void registerFormat(DocumentFormat format,
                        const std::vector<std::wstring>& extensions,
                        DocumentCreator creator);

    std::unique_ptr<IDocument> createDocument(DocumentFormat format) const;
    DocumentFormat detectFormat(const std::wstring& path) const;
    bool isSupported(const std::wstring& path) const;
    std::vector<std::wstring> supportedExtensions() const;

    // Auto-registration helper
    struct AutoRegister {
        AutoRegister(DocumentFormat format,
                     const std::vector<std::wstring>& extensions,
                     DocumentCreator creator) {
            FormatRegistry::instance().registerFormat(format, extensions, creator);
        }
    };

private:
    FormatRegistry() = default;

    struct FormatEntry {
        DocumentFormat format;
        std::vector<std::wstring> extensions;
        DocumentCreator creator;
    };
    std::vector<FormatEntry> m_formats;
};

} // namespace docvision
