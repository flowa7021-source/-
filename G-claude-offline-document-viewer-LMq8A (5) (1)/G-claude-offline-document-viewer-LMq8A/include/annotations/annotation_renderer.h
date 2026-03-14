#pragma once

#include "annotations/annotation_manager.h"
#include "core/document.h"

namespace docvision {

// Annotation renderer — draws annotations on top of page bitmaps
class AnnotationRenderer {
public:
    AnnotationRenderer() = default;

    // Render all visible annotations for a page onto a bitmap
    void renderAnnotations(PageBitmap& target,
                            const std::vector<const Annotation*>& annotations,
                            double scale,
                            const Rect& visibleRegion);

    // Render a single annotation
    void renderAnnotation(PageBitmap& target,
                           const Annotation& annotation,
                           double scale);

    // Render selection handles for selected annotation
    void renderSelectionHandles(PageBitmap& target,
                                 const Annotation& annotation,
                                 double scale);

    // Render annotation creation preview (while drawing)
    void renderPreview(PageBitmap& target,
                        AnnotationType type,
                        const Rect& bounds,
                        const Color& color,
                        double scale);

    // Render ink stroke preview
    void renderInkPreview(PageBitmap& target,
                           const std::vector<InkPoint>& points,
                           const Color& color,
                           float width,
                           double scale);

private:
    void drawHighlight(PageBitmap& target, const Annotation& ann, double scale);
    void drawUnderline(PageBitmap& target, const Annotation& ann, double scale);
    void drawStrikethrough(PageBitmap& target, const Annotation& ann, double scale);
    void drawStickyNote(PageBitmap& target, const Annotation& ann, double scale);
    void drawInk(PageBitmap& target, const Annotation& ann, double scale);
    void drawFreeText(PageBitmap& target, const Annotation& ann, double scale);
    void drawStamp(PageBitmap& target, const Annotation& ann, double scale);
    void drawShape(PageBitmap& target, const Annotation& ann, double scale);
};

} // namespace docvision
