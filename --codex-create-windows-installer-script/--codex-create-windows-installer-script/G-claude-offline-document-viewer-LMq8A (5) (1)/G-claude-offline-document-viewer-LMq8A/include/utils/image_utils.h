#pragma once

#include "core/document.h"
#include <string>
#include <vector>

namespace docvision {
namespace utils {

// Copy bitmap region to clipboard as PNG
bool copyBitmapToClipboard(void* hwnd, const PageBitmap& bitmap);
bool copyBitmapToClipboard(void* hwnd, const PageBitmap& bitmap, const Rect& region);

// Save bitmap to file
bool saveBitmapAsPNG(const PageBitmap& bitmap, const std::wstring& path);
bool saveBitmapAsJPEG(const PageBitmap& bitmap, const std::wstring& path, int quality = 90);

// Extract region from bitmap
PageBitmap extractRegion(const PageBitmap& source, const Rect& region, double scale = 1.0);

// Scale bitmap
PageBitmap scaleBitmap(const PageBitmap& source, int targetWidth, int targetHeight);

// Apply brightness/contrast/gamma
PageBitmap adjustBrightness(const PageBitmap& source, float brightness);  // -1 to 1
PageBitmap adjustContrast(const PageBitmap& source, float contrast);      // 0 to 3
PageBitmap adjustGamma(const PageBitmap& source, float gamma);            // 0.1 to 5

// Invert colors
PageBitmap invertColors(const PageBitmap& source);

// Convert to grayscale
PageBitmap toGrayscale(const PageBitmap& source);

// Encode bitmap as PNG bytes (for clipboard / export)
std::vector<uint8_t> encodePNG(const PageBitmap& bitmap);
std::vector<uint8_t> encodeJPEG(const PageBitmap& bitmap, int quality = 90);

} // namespace utils
} // namespace docvision
