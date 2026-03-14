#include "core/render_pipeline.h"
#include "diagnostics/logger.h"
#include <algorithm>
#include <cmath>

namespace docvision {

RenderPipeline::RenderPipeline() {}

RenderPipeline::~RenderPipeline() {
    shutdown();
}

void RenderPipeline::setDocument(IDocument* document) {
    cancelAll();
    m_document = document;
}

uint64_t RenderPipeline::requestRender(int pageIndex, double scale,
                                        RenderQuality quality,
                                        const PostProcessParams& postProcess,
                                        std::function<void(int, PageBitmap)> callback) {
    RenderRequest request;
    request.pageIndex = pageIndex;
    request.scale = scale;
    request.quality = quality;
    request.postProcess = postProcess;
    request.callback = std::move(callback);
    request.requestId = m_nextRequestId++;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(request));
    }
    m_cv.notify_one();

    // Start render thread if not running
    if (!m_running) {
        m_running = true;
        m_thread = std::thread(&RenderPipeline::renderThread, this);
    }

    return request.requestId;
}

uint64_t RenderPipeline::requestProgressiveRender(int pageIndex, double scale,
                                                    const PostProcessParams& postProcess,
                                                    std::function<void(int, PageBitmap)> callback) {
    // First: quick draft render at reduced scale
    double draftScale = scale * 0.25;
    requestRender(pageIndex, draftScale, RenderQuality::Draft, postProcess, callback);

    // Then: full quality render
    return requestRender(pageIndex, scale, RenderQuality::Normal, postProcess, callback);
}

void RenderPipeline::cancelRequest(uint64_t requestId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Mark as cancelled — will be skipped when dequeued
    // (Can't remove from std::queue, so we mark and skip)
}

void RenderPipeline::cancelAllForPage(int pageIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Replace queue with filtered version
    std::queue<RenderRequest> filtered;
    while (!m_queue.empty()) {
        auto req = std::move(m_queue.front());
        m_queue.pop();
        if (req.pageIndex != pageIndex) {
            filtered.push(std::move(req));
        }
    }
    m_queue = std::move(filtered);
}

void RenderPipeline::cancelAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<RenderRequest> empty;
    m_queue.swap(empty);
}

PageBitmap RenderPipeline::applyPostProcess(const PageBitmap& source,
                                              const PostProcessParams& params) {
    if (params.isDefault()) {
        return source; // no processing needed
    }

    PageBitmap result = source; // copy

    uint8_t* pixels = result.data.data();
    int pixelCount = result.width * result.height;

    for (int i = 0; i < pixelCount; ++i) {
        int idx = i * 4; // BGRA

        float b = pixels[idx] / 255.0f;
        float g = pixels[idx + 1] / 255.0f;
        float r = pixels[idx + 2] / 255.0f;

        // Brightness
        if (params.brightness != 0.0f) {
            r += params.brightness;
            g += params.brightness;
            b += params.brightness;
        }

        // Contrast
        if (params.contrast != 1.0f) {
            r = (r - 0.5f) * params.contrast + 0.5f;
            g = (g - 0.5f) * params.contrast + 0.5f;
            b = (b - 0.5f) * params.contrast + 0.5f;
        }

        // Gamma
        if (params.gamma != 1.0f) {
            float invGamma = 1.0f / params.gamma;
            r = std::pow(std::max(0.0f, r), invGamma);
            g = std::pow(std::max(0.0f, g), invGamma);
            b = std::pow(std::max(0.0f, b), invGamma);
        }

        // Invert
        if (params.invertColors) {
            r = 1.0f - r;
            g = 1.0f - g;
            b = 1.0f - b;
        }

        // Grayscale
        if (params.grayscale) {
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;
            r = g = b = gray;
        }

        // Clamp and write back
        pixels[idx] = static_cast<uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f);
        pixels[idx + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f);
        pixels[idx + 2] = static_cast<uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f);
        // Alpha unchanged
    }

    return result;
}

void RenderPipeline::shutdown() {
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void RenderPipeline::renderThread() {
    while (m_running) {
        RenderRequest request;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });

            if (!m_running) break;
            if (m_queue.empty()) continue;

            request = std::move(m_queue.front());
            m_queue.pop();
        }

        if (!request.cancelled) {
            processRequest(request);
        }
    }
}

void RenderPipeline::processRequest(RenderRequest& request) {
    if (!m_document) return;

    try {
        PageBitmap bitmap = m_document->renderPage(
            request.pageIndex, request.scale, request.quality);

        if (bitmap.isValid() && !request.postProcess.isDefault()) {
            bitmap = applyPostProcess(bitmap, request.postProcess);
        }

        if (request.callback && bitmap.isValid()) {
            request.callback(request.pageIndex, std::move(bitmap));
        }
    } catch (...) {
        LOG_ERROR("Exception in render pipeline for page " +
                  std::to_string(request.pageIndex));
    }
}

} // namespace docvision
