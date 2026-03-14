#include "print/print_manager.h"
#include "annotations/annotation_manager.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"
#include <commdlg.h>
#include <winspool.h>
#include <sstream>

namespace docvision {

PrintManager::PrintManager() {}
PrintManager::~PrintManager() {}

std::vector<int> PrintManager::parsePageRange(const std::wstring& range, int pageCount) {
    std::vector<int> pages;
    if (range.empty()) return pages;

    auto parts = utils::split(range, L',');
    for (const auto& part : parts) {
        auto trimmed = utils::trim(part);
        auto dashPos = trimmed.find(L'-');
        if (dashPos != std::wstring::npos) {
            int from = std::stoi(trimmed.substr(0, dashPos)) - 1;
            int to = std::stoi(trimmed.substr(dashPos + 1)) - 1;
            from = std::max(0, std::min(from, pageCount - 1));
            to = std::max(0, std::min(to, pageCount - 1));
            for (int i = from; i <= to; ++i) {
                pages.push_back(i);
            }
        } else {
            int page = std::stoi(trimmed) - 1;
            if (page >= 0 && page < pageCount) {
                pages.push_back(page);
            }
        }
    }
    return pages;
}

bool PrintManager::showPrintDialog(HWND parentHwnd,
                                    IDocument* document,
                                    AnnotationManager* annotationManager) {
    if (!document) return false;

    PRINTDLGW pd{};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = parentHwnd;
    pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE;
    pd.nMinPage = 1;
    pd.nMaxPage = static_cast<WORD>(document->getPageCount());
    pd.nFromPage = 1;
    pd.nToPage = pd.nMaxPage;

    if (!PrintDlgW(&pd)) {
        return false;
    }

    HDC printerDC = pd.hDC;
    if (!printerDC) return false;

    PrintOptions options;
    if (pd.Flags & PD_SELECTION) {
        options.range = PrintOptions::PageRange::Selection;
    } else if (pd.Flags & PD_PAGENUMS) {
        options.range = PrintOptions::PageRange::Custom;
        options.customRange = std::to_wstring(pd.nFromPage) + L"-" + std::to_wstring(pd.nToPage);
        options.pageIndices = parsePageRange(options.customRange, document->getPageCount());
    } else {
        options.range = PrintOptions::PageRange::All;
    }

    options.copies = pd.nCopies;

    // Resolve page indices
    std::vector<int> pagesToPrint;
    if (options.range == PrintOptions::PageRange::All) {
        for (int i = 0; i < document->getPageCount(); ++i)
            pagesToPrint.push_back(i);
    } else if (options.range == PrintOptions::PageRange::Custom) {
        pagesToPrint = options.pageIndices;
    }

    DOCINFOW di{};
    di.cbSize = sizeof(di);
    std::wstring docName = L"DocVision - " + document->getMetadata().title;
    di.lpszDocName = docName.c_str();

    if (StartDocW(printerDC, &di) <= 0) {
        DeleteDC(printerDC);
        return false;
    }

    int totalPages = static_cast<int>(pagesToPrint.size());
    for (int i = 0; i < totalPages; ++i) {
        if (m_onProgress) {
            m_onProgress(i + 1, totalPages);
        }
        if (!printPage(printerDC, document, annotationManager, pagesToPrint[i], options)) {
            LOG_WARNING("Failed to print page " + std::to_string(pagesToPrint[i] + 1));
        }
    }

    EndDoc(printerDC);
    DeleteDC(printerDC);

    if (pd.hDevMode) GlobalFree(pd.hDevMode);
    if (pd.hDevNames) GlobalFree(pd.hDevNames);

    LOG_INFO("Print job completed: " + std::to_string(totalPages) + " pages");
    return true;
}

bool PrintManager::print(IDocument* document,
                          AnnotationManager* annotationManager,
                          const PrintOptions& options) {
    if (!document) return false;
    LOG_INFO("Direct print not yet implemented, use showPrintDialog");
    return false;
}

std::vector<PrintManager::PreviewPage> PrintManager::generatePreview(
        IDocument* document,
        AnnotationManager* annotationManager,
        const PrintOptions& options,
        double previewScale) {
    std::vector<PreviewPage> previews;
    if (!document) return previews;

    std::vector<int> pagesToPreview;
    if (options.range == PrintOptions::PageRange::All) {
        int count = std::min(document->getPageCount(), 20); // limit preview
        for (int i = 0; i < count; ++i)
            pagesToPreview.push_back(i);
    } else if (options.range == PrintOptions::PageRange::Custom) {
        pagesToPreview = options.pageIndices;
    } else if (options.range == PrintOptions::PageRange::CurrentPage) {
        pagesToPreview = options.pageIndices;
    }

    for (int pageIdx : pagesToPreview) {
        PreviewPage pp;
        pp.pageIndex = pageIdx;
        pp.bitmap = document->renderPage(pageIdx, previewScale, RenderQuality::Draft);
        previews.push_back(std::move(pp));
    }

    return previews;
}

bool PrintManager::printPage(HDC printerDC, IDocument* document,
                              AnnotationManager* /*annotationManager*/,
                              int pageIndex, const PrintOptions& options) {
    if (StartPage(printerDC) <= 0) return false;

    int printerWidth = GetDeviceCaps(printerDC, HORZRES);
    int printerHeight = GetDeviceCaps(printerDC, VERTRES);
    int dpiX = GetDeviceCaps(printerDC, LOGPIXELSX);

    double scale = static_cast<double>(dpiX) / 72.0;
    if (options.scaling == PrintOptions::Scaling::Custom) {
        scale *= options.customScale;
    }

    auto bitmap = document->renderPage(pageIndex, scale, RenderQuality::HighQuality);
    if (!bitmap.isValid()) {
        EndPage(printerDC);
        return false;
    }

    // Fit to page if needed
    double scaleX = static_cast<double>(printerWidth) / bitmap.width;
    double scaleY = static_cast<double>(printerHeight) / bitmap.height;
    double fitScale = std::min(scaleX, scaleY);
    if (options.scaling == PrintOptions::Scaling::ShrinkToFit && fitScale >= 1.0) {
        fitScale = 1.0;
    }

    int destW = static_cast<int>(bitmap.width * fitScale);
    int destH = static_cast<int>(bitmap.height * fitScale);
    int destX = (printerWidth - destW) / 2;
    int destY = (printerHeight - destH) / 2;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bitmap.width;
    bmi.bmiHeader.biHeight = -bitmap.height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(printerDC, destX, destY, destW, destH,
                  0, 0, bitmap.width, bitmap.height,
                  bitmap.data.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

    EndPage(printerDC);
    return true;
}

} // namespace docvision
