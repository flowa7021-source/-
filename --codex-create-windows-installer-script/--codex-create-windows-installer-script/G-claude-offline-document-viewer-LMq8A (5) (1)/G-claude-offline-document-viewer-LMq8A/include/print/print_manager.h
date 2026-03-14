#pragma once

#include "core/document.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace docvision {

class AnnotationManager;

// Print options
struct PrintOptions {
    // Page range
    enum class PageRange {
        All,
        CurrentPage,
        Selection,     // selected pages
        Custom         // custom range string e.g., "1-5,7,10-15"
    };
    PageRange range = PageRange::All;
    std::wstring customRange;
    std::vector<int> pageIndices;   // resolved page indices

    // Annotations
    bool printAnnotations = true;

    // Layout
    enum class Scaling {
        ActualSize,
        FitToPage,
        ShrinkToFit,
        Custom
    };
    Scaling scaling = Scaling::ShrinkToFit;
    double customScale = 1.0;

    // Orientation
    bool autoRotate = true;

    // Copies
    int copies = 1;
    bool collate = true;
};

// Print manager — Windows printing integration
class PrintManager {
public:
    PrintManager();
    ~PrintManager();

    // Show print dialog and print
    bool showPrintDialog(HWND parentHwnd,
                          IDocument* document,
                          AnnotationManager* annotationManager);

    // Print directly (without dialog)
    bool print(IDocument* document,
                AnnotationManager* annotationManager,
                const PrintOptions& options);

    // Print preview
    struct PreviewPage {
        PageBitmap bitmap;
        int pageIndex;
    };
    std::vector<PreviewPage> generatePreview(IDocument* document,
                                               AnnotationManager* annotationManager,
                                               const PrintOptions& options,
                                               double previewScale = 0.5);

    // Parse page range string (e.g., "1-5,7,10-15")
    static std::vector<int> parsePageRange(const std::wstring& range, int pageCount);

    // Progress
    using ProgressCallback = std::function<void(int currentPage, int totalPages)>;
    void setProgressCallback(ProgressCallback cb) { m_onProgress = cb; }

private:
    bool printPage(HDC printerDC, IDocument* document,
                    AnnotationManager* annotationManager,
                    int pageIndex, const PrintOptions& options);

    ProgressCallback m_onProgress;
};

} // namespace docvision
