#pragma once

#include "core/document.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace docvision {

// Annotation types
enum class AnnotationType {
    Highlight,
    Underline,
    Strikethrough,
    StickyNote,
    Ink,            // freehand drawing
    FreeText,       // text box
    Stamp,
    Rectangle,
    Circle,
    Line,
    Arrow
};

// Annotation flags (PDF-compatible)
enum AnnotationFlags : uint32_t {
    ANNOT_FLAG_NONE           = 0,
    ANNOT_FLAG_INVISIBLE      = 0x01,
    ANNOT_FLAG_HIDDEN         = 0x02,
    ANNOT_FLAG_PRINT          = 0x04,
    ANNOT_FLAG_NO_ZOOM        = 0x08,
    ANNOT_FLAG_NO_ROTATE      = 0x10,
    ANNOT_FLAG_LOCKED         = 0x20,    // cannot delete or move
    ANNOT_FLAG_LOCKED_CONTENTS= 0x40,    // cannot change content
    ANNOT_FLAG_READ_ONLY      = 0x80
};

// Color (RGBA)
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    static Color fromRGB(uint8_t r, uint8_t g, uint8_t b) { return {r, g, b, 255}; }
    static Color yellow() { return {255, 255, 0, 128}; }
    static Color red() { return {255, 0, 0, 255}; }
    static Color blue() { return {0, 0, 255, 255}; }
    static Color green() { return {0, 128, 0, 255}; }
};

// Ink point
struct InkPoint {
    double x, y;
    float pressure = 1.0f;
};

// Annotation data
struct Annotation {
    int id = 0;
    AnnotationType type = AnnotationType::Highlight;
    int pageIndex = 0;
    Rect bounds;
    Color color;
    float opacity = 1.0f;
    float borderWidth = 1.0f;
    uint32_t flags = ANNOT_FLAG_PRINT;

    // Type-specific data
    std::wstring text;                       // note content, free text content
    std::vector<Rect> quadPoints;            // highlight/underline regions
    std::vector<std::vector<InkPoint>> inkPaths;  // ink strokes
    std::wstring stampName;                  // stamp type
    std::wstring author;
    std::wstring creationDate;
    std::wstring modificationDate;

    // State
    bool isLocked() const { return (flags & ANNOT_FLAG_LOCKED) != 0; }
    bool isLockedContents() const { return (flags & ANNOT_FLAG_LOCKED_CONTENTS) != 0; }
    bool isHidden() const { return (flags & ANNOT_FLAG_HIDDEN) != 0; }
    void setLocked(bool locked) {
        if (locked) flags |= ANNOT_FLAG_LOCKED; else flags &= ~ANNOT_FLAG_LOCKED;
    }
    void setLockedContents(bool locked) {
        if (locked) flags |= ANNOT_FLAG_LOCKED_CONTENTS; else flags &= ~ANNOT_FLAG_LOCKED_CONTENTS;
    }
};

// Annotation filter
struct AnnotationFilter {
    std::vector<AnnotationType> types;
    int pageIndex = -1;         // -1 = all pages
    bool includeHidden = false;
};

// Annotation manager — unified API across all formats
class AnnotationManager {
public:
    AnnotationManager();
    ~AnnotationManager();

    void setDocument(IDocument* document);

    // CRUD operations
    int addAnnotation(const Annotation& annotation);
    bool updateAnnotation(int annotationId, const Annotation& annotation);
    bool deleteAnnotation(int annotationId);
    const Annotation* getAnnotation(int annotationId) const;

    // Query
    std::vector<const Annotation*> getAnnotations(const AnnotationFilter& filter = {}) const;
    std::vector<const Annotation*> getAnnotationsForPage(int pageIndex) const;
    int getAnnotationCount() const;

    // Hit testing
    const Annotation* hitTest(int pageIndex, double x, double y) const;

    // Lock / Freeze
    bool lockAnnotation(int annotationId);
    bool unlockAnnotation(int annotationId);
    bool lockContents(int annotationId);
    bool unlockContents(int annotationId);

    // Flatten (burn into page)
    bool flattenAnnotation(int annotationId);
    bool flattenAll();
    bool flattenPage(int pageIndex);

    // Visibility
    void setAnnotationsVisible(bool visible);
    bool areAnnotationsVisible() const { return m_visible; }

    // Persistence
    bool saveAnnotations();
    bool loadAnnotations();
    bool exportAnnotations(const std::wstring& path) const;
    bool importAnnotations(const std::wstring& path);

    // Sidecar storage (for formats that don't support native annotations)
    bool saveSidecar(const std::wstring& documentPath) const;
    bool loadSidecar(const std::wstring& documentPath);
    std::wstring getSidecarPath(const std::wstring& documentPath) const;

    // Callbacks
    using AnnotationChangedCallback = std::function<void(int annotationId)>;
    void onAnnotationAdded(AnnotationChangedCallback cb) { m_onAdded = cb; }
    void onAnnotationUpdated(AnnotationChangedCallback cb) { m_onUpdated = cb; }
    void onAnnotationDeleted(AnnotationChangedCallback cb) { m_onDeleted = cb; }

private:
    IDocument* m_document = nullptr;
    std::vector<Annotation> m_annotations;
    int m_nextId = 1;
    bool m_visible = true;

    AnnotationChangedCallback m_onAdded;
    AnnotationChangedCallback m_onUpdated;
    AnnotationChangedCallback m_onDeleted;
};

} // namespace docvision
