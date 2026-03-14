#include "core/page_cache.h"
#include <algorithm>

namespace docvision {

PageCache::PageCache(size_t maxMemoryBytes)
    : m_maxMemory(maxMemoryBytes) {}

void PageCache::put(const CacheKey& key, PageBitmap bitmap) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Remove existing entry if present
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_currentMemory -= bitmapMemory(it->second.bitmap);
        m_lruList.erase(it->second.lruIterator);
        m_cache.erase(it);
    }

    size_t newItemMemory = bitmapMemory(bitmap);

    // Evict until there's room
    while (m_currentMemory + newItemMemory > m_maxMemory && !m_lruList.empty()) {
        evict();
    }

    // Insert new entry
    m_lruList.push_front(key);
    CacheEntry entry;
    entry.bitmap = std::move(bitmap);
    entry.lruIterator = m_lruList.begin();
    m_currentMemory += newItemMemory;
    m_cache[key] = std::move(entry);
}

const PageBitmap* PageCache::get(const CacheKey& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        ++m_misses;
        return nullptr;
    }

    ++m_hits;

    // Move to front of LRU
    m_lruList.erase(it->second.lruIterator);
    m_lruList.push_front(key);
    it->second.lruIterator = m_lruList.begin();

    return &it->second.bitmap;
}

bool PageCache::contains(const CacheKey& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.find(key) != m_cache.end();
}

void PageCache::remove(const CacheKey& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_currentMemory -= bitmapMemory(it->second.bitmap);
        m_lruList.erase(it->second.lruIterator);
        m_cache.erase(it);
    }
}

void PageCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_lruList.clear();
    m_currentMemory = 0;
}

void PageCache::setMaxMemory(size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxMemory = bytes;
    while (m_currentMemory > m_maxMemory && !m_lruList.empty()) {
        evict();
    }
}

void PageCache::evict() {
    // Remove least recently used (back of list)
    if (m_lruList.empty()) return;

    CacheKey lruKey = m_lruList.back();
    auto it = m_cache.find(lruKey);
    if (it != m_cache.end()) {
        m_currentMemory -= bitmapMemory(it->second.bitmap);
        m_cache.erase(it);
    }
    m_lruList.pop_back();
}

size_t PageCache::bitmapMemory(const PageBitmap& bmp) const {
    return bmp.data.size();
}

} // namespace docvision
