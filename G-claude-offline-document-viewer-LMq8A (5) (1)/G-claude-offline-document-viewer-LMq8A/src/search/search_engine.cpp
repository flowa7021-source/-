#include "search/search_engine.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <regex>

namespace docvision {

SearchEngine::SearchEngine() {}
SearchEngine::~SearchEngine() {}

void SearchEngine::setDocument(IDocument* document) {
    m_document = document;
    clearSearch();
}

void SearchEngine::setOCRPipeline(OCRPipeline* ocrPipeline) {
    m_ocrPipeline = ocrPipeline;
}

std::vector<SearchResult> SearchEngine::search(const std::wstring& query,
                                                const SearchOptions& options) {
    startSearch(query, options);
    return m_results;
}

void SearchEngine::startSearch(const std::wstring& query, const SearchOptions& options) {
    clearSearch();
    m_query = query;
    m_options = options;

    if (!m_document || query.empty()) {
        return;
    }

    LOG_INFO("Starting search for: " + utils::wideToUtf8(query));

    int pageCount = m_document->getPageCount();

    for (int i = 0; i < pageCount; ++i) {
        // Native text search
        if (m_document->hasTextLayer(i)) {
            auto results = m_document->search(query, options.caseSensitive, options.wholeWord);
            for (auto& r : results) {
                if (r.pageIndex == i) {
                    m_results.push_back(r);
                }
            }
        }
    }

    // Sort by page and position
    std::sort(m_results.begin(), m_results.end(), [](const SearchResult& a, const SearchResult& b) {
        if (a.pageIndex != b.pageIndex) return a.pageIndex < b.pageIndex;
        if (a.bounds.y != b.bounds.y) return a.bounds.y < b.bounds.y;
        return a.bounds.x < b.bounds.x;
    });

    LOG_INFO("Search found " + std::to_string(m_results.size()) + " results");

    if (m_onResultsChanged) {
        m_onResultsChanged(static_cast<int>(m_results.size()));
    }

    if (!m_results.empty()) {
        m_currentIndex = 0;
    }
}

const SearchResult* SearchEngine::findNext() {
    if (m_results.empty()) return nullptr;
    m_currentIndex++;
    if (m_currentIndex >= static_cast<int>(m_results.size())) {
        if (m_options.wrapAround) {
            m_currentIndex = 0;
        } else {
            m_currentIndex = static_cast<int>(m_results.size()) - 1;
            return nullptr;
        }
    }
    return &m_results[m_currentIndex];
}

const SearchResult* SearchEngine::findPrevious() {
    if (m_results.empty()) return nullptr;
    m_currentIndex--;
    if (m_currentIndex < 0) {
        if (m_options.wrapAround) {
            m_currentIndex = static_cast<int>(m_results.size()) - 1;
        } else {
            m_currentIndex = 0;
            return nullptr;
        }
    }
    return &m_results[m_currentIndex];
}

const SearchResult* SearchEngine::currentResult() const {
    if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_results.size())) {
        return &m_results[m_currentIndex];
    }
    return nullptr;
}

void SearchEngine::clearSearch() {
    m_query.clear();
    m_results.clear();
    m_currentIndex = -1;
}

std::vector<Rect> SearchEngine::getHighlightsForPage(int pageIndex) const {
    std::vector<Rect> rects;
    for (const auto& r : m_results) {
        if (r.pageIndex == pageIndex) {
            rects.push_back(r.bounds);
        }
    }
    return rects;
}

} // namespace docvision
