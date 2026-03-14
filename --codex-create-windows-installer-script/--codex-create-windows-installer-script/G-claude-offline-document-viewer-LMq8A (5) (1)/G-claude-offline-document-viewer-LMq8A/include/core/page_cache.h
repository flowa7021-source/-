#pragma once

#include "core/document.h"
#include <unordered_map>
#include <list>
#include <mutex>
#include <cstdint>

namespace docvision {

// LRU page cache with configurable memory budget
class PageCache {
public:
    explicit PageCache(size_t maxMemoryBytes = 300 * 1024 * 1024); // 300 MB default
    ~PageCache() = default;

    // Cache key: page index + scale + quality
    struct CacheKey {
        int pageIndex;
        int scalePercent; // scale * 100, for hashing
        RenderQuality quality;

        bool operator==(const CacheKey& other) const {
            return pageIndex == other.pageIndex &&
                   scalePercent == other.scalePercent &&
                   quality == other.quality;
        }
    };

    struct CacheKeyHash {
        size_t operator()(const CacheKey& key) const {
            size_t h = std::hash<int>()(key.pageIndex);
            h ^= std::hash<int>()(key.scalePercent) << 1;
            h ^= std::hash<int>()(static_cast<int>(key.quality)) << 2;
            return h;
        }
    };

    // Store a rendered page
    void put(const CacheKey& key, PageBitmap bitmap);

    // Retrieve a cached page (returns nullptr if not cached)
    const PageBitmap* get(const CacheKey& key);

    // Check if page is cached
    bool contains(const CacheKey& key) const;

    // Remove specific page
    void remove(const CacheKey& key);

    // Clear all cached pages
    void clear();

    // Memory management
    size_t currentMemoryUsage() const { return m_currentMemory; }
    size_t maxMemory() const { return m_maxMemory; }
    void setMaxMemory(size_t bytes);

    // Statistics
    size_t hitCount() const { return m_hits; }
    size_t missCount() const { return m_misses; }
    size_t entryCount() const { return m_cache.size(); }

private:
    void evict();
    size_t bitmapMemory(const PageBitmap& bmp) const;

    size_t m_maxMemory;
    size_t m_currentMemory = 0;
    size_t m_hits = 0;
    size_t m_misses = 0;

    // LRU list: front = most recently used
    using LRUList = std::list<CacheKey>;
    LRUList m_lruList;

    struct CacheEntry {
        PageBitmap bitmap;
        LRUList::iterator lruIterator;
    };

    std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> m_cache;
    mutable std::mutex m_mutex;
};

} // namespace docvision
