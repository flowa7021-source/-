#include "annotations/annotation_renderer.h"
#include <algorithm>
#include <cmath>

namespace docvision {

void AnnotationRenderer::renderAnnotations(PageBitmap& target,
                                             const std::vector<const Annotation*>& annotations,
                                             double scale,
                                             const Rect& /*visibleRegion*/) {
    for (const auto* ann : annotations) {
        if (!ann->isHidden()) {
            renderAnnotation(target, *ann, scale);
        }
    }
}

void AnnotationRenderer::renderAnnotation(PageBitmap& target,
                                            const Annotation& annotation,
                                            double scale) {
    switch (annotation.type) {
        case AnnotationType::Highlight:     drawHighlight(target, annotation, scale); break;
        case AnnotationType::Underline:     drawUnderline(target, annotation, scale); break;
        case AnnotationType::Strikethrough: drawStrikethrough(target, annotation, scale); break;
        case AnnotationType::StickyNote:    drawStickyNote(target, annotation, scale); break;
        case AnnotationType::Ink:           drawInk(target, annotation, scale); break;
        case AnnotationType::FreeText:      drawFreeText(target, annotation, scale); break;
        case AnnotationType::Stamp:         drawStamp(target, annotation, scale); break;
        case AnnotationType::Rectangle:
        case AnnotationType::Circle:
        case AnnotationType::Line:
        case AnnotationType::Arrow:         drawShape(target, annotation, scale); break;
    }
}

// Helper: blend a color onto a pixel (premultiplied alpha)
static void blendPixel(uint8_t* dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    float alpha = a / 255.0f;
    float invAlpha = 1.0f - alpha;
    dst[0] = static_cast<uint8_t>(b * alpha + dst[0] * invAlpha);
    dst[1] = static_cast<uint8_t>(g * alpha + dst[1] * invAlpha);
    dst[2] = static_cast<uint8_t>(r * alpha + dst[2] * invAlpha);
}

// Fill a rectangle with semi-transparent color
static void fillRect(PageBitmap& target, int x0, int y0, int x1, int y1,
                     uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(target.width, x1);
    y1 = std::min(target.height, y1);

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            int idx = y * target.stride + x * 4;
            blendPixel(&target.data[idx], r, g, b, a);
        }
    }
}

// Draw a horizontal line
static void drawHLine(PageBitmap& target, int x0, int x1, int y, int thickness,
                      uint8_t r, uint8_t g, uint8_t b) {
    fillRect(target, x0, y, x1, y + thickness, r, g, b, 255);
}

void AnnotationRenderer::drawHighlight(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>(ann.bounds.y * scale);
    int w = static_cast<int>(ann.bounds.width * scale);
    int h = static_cast<int>(ann.bounds.height * scale);

    fillRect(target, x, y, x + w, y + h,
             ann.color.r, ann.color.g, ann.color.b,
             static_cast<uint8_t>(ann.opacity * ann.color.a));
}

void AnnotationRenderer::drawUnderline(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>((ann.bounds.y + ann.bounds.height) * scale);
    int w = static_cast<int>(ann.bounds.width * scale);
    int thickness = std::max(1, static_cast<int>(2 * scale));

    drawHLine(target, x, x + w, y - thickness, thickness,
              ann.color.r, ann.color.g, ann.color.b);
}

void AnnotationRenderer::drawStrikethrough(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>((ann.bounds.y + ann.bounds.height * 0.5) * scale);
    int w = static_cast<int>(ann.bounds.width * scale);
    int thickness = std::max(1, static_cast<int>(2 * scale));

    drawHLine(target, x, x + w, y, thickness, ann.color.r, ann.color.g, ann.color.b);
}

void AnnotationRenderer::drawStickyNote(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>(ann.bounds.y * scale);
    int size = static_cast<int>(20 * scale);

    fillRect(target, x, y, x + size, y + size, 255, 255, 0, 200);
    // Border
    fillRect(target, x, y, x + size, y + 1, 200, 200, 0, 255);
    fillRect(target, x, y, x + 1, y + size, 200, 200, 0, 255);
    fillRect(target, x + size - 1, y, x + size, y + size, 200, 200, 0, 255);
    fillRect(target, x, y + size - 1, x + size, y + size, 200, 200, 0, 255);
}

void AnnotationRenderer::drawInk(PageBitmap& target, const Annotation& ann, double scale) {
    for (const auto& path : ann.inkPaths) {
        for (size_t i = 1; i < path.size(); ++i) {
            int x0 = static_cast<int>(path[i-1].x * scale);
            int y0 = static_cast<int>(path[i-1].y * scale);
            int x1 = static_cast<int>(path[i].x * scale);
            int y1 = static_cast<int>(path[i].y * scale);

            // Bresenham line drawing
            int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
            int err = dx - dy;

            int thickness = std::max(1, static_cast<int>(ann.borderWidth * scale));
            while (true) {
                fillRect(target, x0 - thickness/2, y0 - thickness/2,
                         x0 + thickness/2 + 1, y0 + thickness/2 + 1,
                         ann.color.r, ann.color.g, ann.color.b, ann.color.a);

                if (x0 == x1 && y0 == y1) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x0 += sx; }
                if (e2 < dx) { err += dx; y0 += sy; }
            }
        }
    }
}

void AnnotationRenderer::drawFreeText(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>(ann.bounds.y * scale);
    int w = static_cast<int>(ann.bounds.width * scale);
    int h = static_cast<int>(ann.bounds.height * scale);

    // Background
    fillRect(target, x, y, x + w, y + h, 255, 255, 255, 200);
    // Border
    fillRect(target, x, y, x + w, y + 1, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x, y + h - 1, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x, y, x + 1, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x + w - 1, y, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    // Text rendering would use GDI/DirectWrite in production
}

void AnnotationRenderer::drawStamp(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>(ann.bounds.y * scale);
    int w = static_cast<int>(ann.bounds.width * scale);
    int h = static_cast<int>(ann.bounds.height * scale);

    fillRect(target, x, y, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 100);
    // Border
    int thickness = std::max(2, static_cast<int>(3 * scale));
    fillRect(target, x, y, x + w, y + thickness, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x, y + h - thickness, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x, y, x + thickness, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x + w - thickness, y, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
}

void AnnotationRenderer::drawShape(PageBitmap& target, const Annotation& ann, double scale) {
    int x = static_cast<int>(ann.bounds.x * scale);
    int y = static_cast<int>(ann.bounds.y * scale);
    int w = static_cast<int>(ann.bounds.width * scale);
    int h = static_cast<int>(ann.bounds.height * scale);
    int thickness = std::max(1, static_cast<int>(ann.borderWidth * scale));

    // Draw border rectangle (all shape types use rect for now)
    fillRect(target, x, y, x + w, y + thickness, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x, y + h - thickness, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x, y, x + thickness, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
    fillRect(target, x + w - thickness, y, x + w, y + h, ann.color.r, ann.color.g, ann.color.b, 255);
}

void AnnotationRenderer::renderSelectionHandles(PageBitmap& target,
                                                  const Annotation& annotation,
                                                  double scale) {
    int x = static_cast<int>(annotation.bounds.x * scale);
    int y = static_cast<int>(annotation.bounds.y * scale);
    int w = static_cast<int>(annotation.bounds.width * scale);
    int h = static_cast<int>(annotation.bounds.height * scale);
    int hs = 4; // handle size

    // 8 handles around the bounding box
    int handles[][2] = {
        {x, y}, {x + w/2, y}, {x + w, y},
        {x, y + h/2}, {x + w, y + h/2},
        {x, y + h}, {x + w/2, y + h}, {x + w, y + h}
    };

    for (auto& hp : handles) {
        fillRect(target, hp[0] - hs, hp[1] - hs, hp[0] + hs, hp[1] + hs, 0, 120, 215, 255);
    }
}

void AnnotationRenderer::renderPreview(PageBitmap& target,
                                         AnnotationType type,
                                         const Rect& bounds,
                                         const Color& color,
                                         double scale) {
    Annotation preview;
    preview.type = type;
    preview.bounds = bounds;
    preview.color = color;
    preview.opacity = 0.5f;
    renderAnnotation(target, preview, scale);
}

void AnnotationRenderer::renderInkPreview(PageBitmap& target,
                                            const std::vector<InkPoint>& points,
                                            const Color& color,
                                            float width,
                                            double scale) {
    Annotation preview;
    preview.type = AnnotationType::Ink;
    preview.color = color;
    preview.borderWidth = width;
    preview.inkPaths.push_back(points);
    renderAnnotation(target, preview, scale);
}

} // namespace docvision
