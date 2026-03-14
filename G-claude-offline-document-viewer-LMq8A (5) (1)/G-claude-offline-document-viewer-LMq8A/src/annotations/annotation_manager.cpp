#include "annotations/annotation_manager.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "diagnostics/logger.h"
#include <algorithm>
#include <sstream>

namespace docvision {

AnnotationManager::AnnotationManager() {}
AnnotationManager::~AnnotationManager() {}

void AnnotationManager::setDocument(IDocument* document) {
    m_document = document;
    m_annotations.clear();
    m_nextId = 1;

    if (document && !document->getFilePath().empty()) {
        // Try to load native annotations (PDF) or sidecar
        if (document->supportsAnnotations()) {
            loadAnnotations();
        } else {
            loadSidecar(document->getFilePath());
        }
    }
}

int AnnotationManager::addAnnotation(const Annotation& annotation) {
    Annotation ann = annotation;
    ann.id = m_nextId++;
    m_annotations.push_back(ann);
    if (m_onAdded) m_onAdded(ann.id);
    return ann.id;
}

bool AnnotationManager::updateAnnotation(int annotationId, const Annotation& annotation) {
    for (auto& ann : m_annotations) {
        if (ann.id == annotationId) {
            if (ann.isLocked()) return false;
            if (ann.isLockedContents()) {
                // Can update position/color but not content
                ann.bounds = annotation.bounds;
                ann.color = annotation.color;
                ann.opacity = annotation.opacity;
            } else {
                int id = ann.id;
                uint32_t flags = ann.flags; // preserve lock state
                ann = annotation;
                ann.id = id;
                ann.flags = flags;
            }
            if (m_onUpdated) m_onUpdated(annotationId);
            return true;
        }
    }
    return false;
}

bool AnnotationManager::deleteAnnotation(int annotationId) {
    auto it = std::find_if(m_annotations.begin(), m_annotations.end(),
                            [annotationId](const Annotation& a) { return a.id == annotationId; });
    if (it == m_annotations.end()) return false;
    if (it->isLocked()) return false;

    m_annotations.erase(it);
    if (m_onDeleted) m_onDeleted(annotationId);
    return true;
}

const Annotation* AnnotationManager::getAnnotation(int annotationId) const {
    for (const auto& ann : m_annotations) {
        if (ann.id == annotationId) return &ann;
    }
    return nullptr;
}

std::vector<const Annotation*> AnnotationManager::getAnnotations(const AnnotationFilter& filter) const {
    std::vector<const Annotation*> result;
    for (const auto& ann : m_annotations) {
        if (!m_visible && !filter.includeHidden) continue;
        if (ann.isHidden() && !filter.includeHidden) continue;
        if (filter.pageIndex >= 0 && ann.pageIndex != filter.pageIndex) continue;
        if (!filter.types.empty()) {
            bool typeMatch = std::find(filter.types.begin(), filter.types.end(), ann.type) != filter.types.end();
            if (!typeMatch) continue;
        }
        result.push_back(&ann);
    }
    return result;
}

std::vector<const Annotation*> AnnotationManager::getAnnotationsForPage(int pageIndex) const {
    AnnotationFilter filter;
    filter.pageIndex = pageIndex;
    return getAnnotations(filter);
}

int AnnotationManager::getAnnotationCount() const {
    return static_cast<int>(m_annotations.size());
}

const Annotation* AnnotationManager::hitTest(int pageIndex, double x, double y) const {
    for (auto it = m_annotations.rbegin(); it != m_annotations.rend(); ++it) {
        if (it->pageIndex == pageIndex && it->bounds.contains(x, y)) {
            return &(*it);
        }
    }
    return nullptr;
}

bool AnnotationManager::lockAnnotation(int annotationId) {
    for (auto& ann : m_annotations) {
        if (ann.id == annotationId) {
            ann.setLocked(true);
            ann.setLockedContents(true);
            if (m_onUpdated) m_onUpdated(annotationId);
            return true;
        }
    }
    return false;
}

bool AnnotationManager::unlockAnnotation(int annotationId) {
    for (auto& ann : m_annotations) {
        if (ann.id == annotationId) {
            ann.setLocked(false);
            if (m_onUpdated) m_onUpdated(annotationId);
            return true;
        }
    }
    return false;
}

bool AnnotationManager::lockContents(int annotationId) {
    for (auto& ann : m_annotations) {
        if (ann.id == annotationId) {
            ann.setLockedContents(true);
            if (m_onUpdated) m_onUpdated(annotationId);
            return true;
        }
    }
    return false;
}

bool AnnotationManager::unlockContents(int annotationId) {
    for (auto& ann : m_annotations) {
        if (ann.id == annotationId) {
            ann.setLockedContents(false);
            if (m_onUpdated) m_onUpdated(annotationId);
            return true;
        }
    }
    return false;
}

bool AnnotationManager::flattenAnnotation(int /*annotationId*/) {
    // In production: burn annotation into page content via MuPDF
    // Then remove from annotation list
    LOG_INFO("Flatten annotation — requires engine support");
    return false;
}

bool AnnotationManager::flattenAll() {
    // Flatten all annotations
    LOG_INFO("Flatten all annotations");
    return false;
}

bool AnnotationManager::flattenPage(int /*pageIndex*/) {
    LOG_INFO("Flatten page annotations");
    return false;
}

void AnnotationManager::setAnnotationsVisible(bool visible) {
    m_visible = visible;
}

bool AnnotationManager::saveAnnotations() {
    if (!m_document) return false;
    if (m_document->supportsAnnotations()) {
        return m_document->save(m_document->getFilePath());
    }
    return saveSidecar(m_document->getFilePath());
}

bool AnnotationManager::loadAnnotations() {
    // In production: load from PDF via MuPDF annotation API
    return true;
}

bool AnnotationManager::saveSidecar(const std::wstring& documentPath) const {
    std::wstring sidecarPath = getSidecarPath(documentPath);

    // Serialize annotations to JSON
    std::ostringstream json;
    json << "{\n  \"annotations\": [\n";
    for (size_t i = 0; i < m_annotations.size(); ++i) {
        const auto& a = m_annotations[i];
        json << "    {\"id\": " << a.id
             << ", \"type\": " << static_cast<int>(a.type)
             << ", \"page\": " << a.pageIndex
             << ", \"x\": " << a.bounds.x << ", \"y\": " << a.bounds.y
             << ", \"w\": " << a.bounds.width << ", \"h\": " << a.bounds.height
             << ", \"flags\": " << a.flags
             << "}";
        if (i + 1 < m_annotations.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n}\n";

    return utils::writeFileText(sidecarPath, json.str());
}

bool AnnotationManager::loadSidecar(const std::wstring& documentPath) {
    std::wstring sidecarPath = getSidecarPath(documentPath);
    if (!utils::fileExists(sidecarPath)) return false;
    // Parse JSON and load annotations
    // TODO: implement JSON parsing
    return true;
}

std::wstring AnnotationManager::getSidecarPath(const std::wstring& documentPath) const {
    return documentPath + L".annotations.json";
}

bool AnnotationManager::exportAnnotations(const std::wstring& path) const {
    return saveSidecar(path);
}

bool AnnotationManager::importAnnotations(const std::wstring& path) {
    return loadSidecar(path);
}

} // namespace docvision
