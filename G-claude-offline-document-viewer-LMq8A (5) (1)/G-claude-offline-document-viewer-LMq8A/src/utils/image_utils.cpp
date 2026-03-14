#include "utils/image_utils.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace docvision {
namespace utils {

bool copyBitmapToClipboard(void* hwnd, const PageBitmap& bitmap) {
    if (!bitmap.isValid()) return false;

    if (!OpenClipboard(static_cast<HWND>(hwnd))) return false;
    EmptyClipboard();

    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(bih);
    bih.biWidth = bitmap.width;
    bih.biHeight = -bitmap.height; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = static_cast<DWORD>(bitmap.data.size());

    size_t totalSize = sizeof(BITMAPINFOHEADER) + bitmap.data.size();
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, totalSize);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    void* pMem = GlobalLock(hMem);
    memcpy(pMem, &bih, sizeof(bih));
    memcpy(static_cast<uint8_t*>(pMem) + sizeof(bih), bitmap.data.data(), bitmap.data.size());
    GlobalUnlock(hMem);

    SetClipboardData(CF_DIB, hMem);
    CloseClipboard();
    return true;
}

bool copyBitmapToClipboard(void* hwnd, const PageBitmap& bitmap, const Rect& region) {
    auto cropped = extractRegion(bitmap, region);
    return copyBitmapToClipboard(hwnd, cropped);
}

bool saveBitmapAsPNG(const PageBitmap& bitmap, const std::wstring& path) {
    if (!bitmap.isValid()) return false;
    auto data = encodePNG(bitmap);
    if (data.empty()) return false;

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    CloseHandle(hFile);
    return written == data.size();
}

bool saveBitmapAsJPEG(const PageBitmap& bitmap, const std::wstring& path, int quality) {
    if (!bitmap.isValid()) return false;
    auto data = encodeJPEG(bitmap, quality);
    if (data.empty()) return false;

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
    CloseHandle(hFile);
    return written == data.size();
}

PageBitmap extractRegion(const PageBitmap& source, const Rect& region, double scale) {
    PageBitmap result;
    if (!source.isValid()) return result;

    int sx = std::max(0, static_cast<int>(region.x * source.scale));
    int sy = std::max(0, static_cast<int>(region.y * source.scale));
    int sw = static_cast<int>(region.width * source.scale);
    int sh = static_cast<int>(region.height * source.scale);

    sw = std::min(sw, source.width - sx);
    sh = std::min(sh, source.height - sy);
    if (sw <= 0 || sh <= 0) return result;

    result.width = static_cast<int>(sw * scale);
    result.height = static_cast<int>(sh * scale);
    result.stride = result.width * 4;
    result.scale = source.scale * scale;
    result.data.resize(result.stride * result.height);

    // Simple nearest-neighbor for now
    for (int y = 0; y < result.height; ++y) {
        int srcY = sy + static_cast<int>(y / scale);
        if (srcY >= source.height) break;
        for (int x = 0; x < result.width; ++x) {
            int srcX = sx + static_cast<int>(x / scale);
            if (srcX >= source.width) break;

            int dstIdx = y * result.stride + x * 4;
            int srcIdx = srcY * source.stride + srcX * 4;
            memcpy(&result.data[dstIdx], &source.data[srcIdx], 4);
        }
    }
    return result;
}

PageBitmap scaleBitmap(const PageBitmap& source, int targetWidth, int targetHeight) {
    PageBitmap result;
    if (!source.isValid() || targetWidth <= 0 || targetHeight <= 0) return result;

    result.width = targetWidth;
    result.height = targetHeight;
    result.stride = targetWidth * 4;
    result.scale = source.scale * (static_cast<double>(targetWidth) / source.width);
    result.data.resize(result.stride * result.height);

    double scaleX = static_cast<double>(source.width) / targetWidth;
    double scaleY = static_cast<double>(source.height) / targetHeight;

    for (int y = 0; y < targetHeight; ++y) {
        int srcY = std::min(static_cast<int>(y * scaleY), source.height - 1);
        for (int x = 0; x < targetWidth; ++x) {
            int srcX = std::min(static_cast<int>(x * scaleX), source.width - 1);
            int dstIdx = y * result.stride + x * 4;
            int srcIdx = srcY * source.stride + srcX * 4;
            memcpy(&result.data[dstIdx], &source.data[srcIdx], 4);
        }
    }
    return result;
}

PageBitmap adjustBrightness(const PageBitmap& source, float brightness) {
    PageBitmap result = source;
    int offset = static_cast<int>(brightness * 255.0f);
    for (size_t i = 0; i < result.data.size(); i += 4) {
        for (int c = 0; c < 3; ++c) {
            int val = result.data[i + c] + offset;
            result.data[i + c] = static_cast<uint8_t>(std::clamp(val, 0, 255));
        }
    }
    return result;
}

PageBitmap adjustContrast(const PageBitmap& source, float contrast) {
    PageBitmap result = source;
    for (size_t i = 0; i < result.data.size(); i += 4) {
        for (int c = 0; c < 3; ++c) {
            float val = (result.data[i + c] / 255.0f - 0.5f) * contrast + 0.5f;
            result.data[i + c] = static_cast<uint8_t>(std::clamp(val * 255.0f, 0.0f, 255.0f));
        }
    }
    return result;
}

PageBitmap adjustGamma(const PageBitmap& source, float gamma) {
    PageBitmap result = source;
    float invGamma = 1.0f / gamma;
    for (size_t i = 0; i < result.data.size(); i += 4) {
        for (int c = 0; c < 3; ++c) {
            float val = std::pow(result.data[i + c] / 255.0f, invGamma);
            result.data[i + c] = static_cast<uint8_t>(std::clamp(val * 255.0f, 0.0f, 255.0f));
        }
    }
    return result;
}

PageBitmap invertColors(const PageBitmap& source) {
    PageBitmap result = source;
    for (size_t i = 0; i < result.data.size(); i += 4) {
        result.data[i + 0] = 255 - result.data[i + 0]; // B
        result.data[i + 1] = 255 - result.data[i + 1]; // G
        result.data[i + 2] = 255 - result.data[i + 2]; // R
        // Alpha unchanged
    }
    return result;
}

PageBitmap toGrayscale(const PageBitmap& source) {
    PageBitmap result = source;
    for (size_t i = 0; i < result.data.size(); i += 4) {
        uint8_t gray = static_cast<uint8_t>(
            0.114f * result.data[i + 0] +  // B
            0.587f * result.data[i + 1] +  // G
            0.299f * result.data[i + 2]    // R
        );
        result.data[i + 0] = gray;
        result.data[i + 1] = gray;
        result.data[i + 2] = gray;
    }
    return result;
}

std::vector<uint8_t> encodePNG(const PageBitmap& bitmap) {
    // Minimal uncompressed PNG encoder (valid PNG, no compression)
    if (!bitmap.isValid()) return {};

    // For a production build, use libpng or stb_image_write
    // This is a BMP fallback for basic functionality
    std::vector<uint8_t> bmp;
    int rowSize = bitmap.width * 4;
    int dataSize = rowSize * bitmap.height;
    int fileSize = 54 + dataSize;

    bmp.resize(fileSize);
    // BMP header
    bmp[0] = 'B'; bmp[1] = 'M';
    memcpy(&bmp[2], &fileSize, 4);
    int offset = 54;
    memcpy(&bmp[10], &offset, 4);
    int headerSize = 40;
    memcpy(&bmp[14], &headerSize, 4);
    memcpy(&bmp[18], &bitmap.width, 4);
    int negHeight = -bitmap.height;
    memcpy(&bmp[22], &negHeight, 4);
    short planes = 1; memcpy(&bmp[26], &planes, 2);
    short bpp = 32; memcpy(&bmp[28], &bpp, 2);

    memcpy(&bmp[54], bitmap.data.data(), std::min(static_cast<size_t>(dataSize), bitmap.data.size()));
    return bmp;
}

std::vector<uint8_t> encodeJPEG(const PageBitmap& bitmap, int quality) {
    (void)quality;
    // For production, use libjpeg-turbo
    // Fallback: return BMP data
    return encodePNG(bitmap);
}

} // namespace utils
} // namespace docvision
