#pragma once

#include "core/document.h"
#include <functional>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

namespace docvision {

// Render request
struct RenderRequest {
    int pageIndex = 0;
    double scale = 1.0;
    RenderQuality quality = RenderQuality::Normal;
    PostProcessParams postProcess;
    std::function<void(int pageIndex, PageBitmap bitmap)> callback;
    uint64_t requestId = 0;
    bool cancelled = false;
};

// Render pipeline — manages async page rendering with progressive display
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    void setDocument(IDocument* document);

    // Submit render request (returns request ID for cancellation)
    uint64_t requestRender(int pageIndex, double scale,
                            RenderQuality quality,
                            const PostProcessParams& postProcess,
                            std::function<void(int, PageBitmap)> callback);

    // Progressive render: first draft, then high quality
    uint64_t requestProgressiveRender(int pageIndex, double scale,
                                       const PostProcessParams& postProcess,
                                       std::function<void(int, PageBitmap)> callback);

    // Cancel pending request
    void cancelRequest(uint64_t requestId);
    void cancelAllForPage(int pageIndex);
    void cancelAll();

    // Apply post-processing to existing bitmap
    static PageBitmap applyPostProcess(const PageBitmap& source,
                                        const PostProcessParams& params);

    void shutdown();

private:
    void renderThread();
    void processRequest(RenderRequest& request);

    IDocument* m_document = nullptr;
    std::thread m_thread;
    std::queue<RenderRequest> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_nextRequestId{1};
};

} // namespace docvision
