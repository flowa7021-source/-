#pragma once

#include "core/document.h"
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace docvision {

class OCRPipeline;

// Search options
struct SearchOptions {
    bool caseSensitive = false;
    bool wholeWord = false;
    bool useRegex = false;
    bool searchOCR = true;          // include OCR index in search
    bool wrapAround = true;
};

// Unified search engine — combines native text search and OCR search
class SearchEngine {
public:
    SearchEngine();
    ~SearchEngine();

    void setDocument(IDocument* document);
    void setOCRPipeline(OCRPipeline* ocrPipeline);

    // Search
    std::vector<SearchResult> search(const std::wstring& query,
                                      const SearchOptions& options = {});

    // Incremental search (find next / find previous)
    void startSearch(const std::wstring& query, const SearchOptions& options = {});
    const SearchResult* findNext();
    const SearchResult* findPrevious();
    const SearchResult* currentResult() const;
    int currentResultIndex() const { return m_currentIndex; }
    int totalResults() const { return static_cast<int>(m_results.size()); }

    // Clear
    void clearSearch();
    bool hasActiveSearch() const { return !m_query.empty(); }
    std::wstring getQuery() const { return m_query; }

    // Results
    const std::vector<SearchResult>& getResults() const { return m_results; }

    // Highlight rectangles for current page
    std::vector<Rect> getHighlightsForPage(int pageIndex) const;

    // Callbacks
    using ResultsChangedCallback = std::function<void(int totalResults)>;
    void onResultsChanged(ResultsChangedCallback cb) { m_onResultsChanged = cb; }

private:
    IDocument* m_document = nullptr;
    OCRPipeline* m_ocrPipeline = nullptr;
    std::wstring m_query;
    SearchOptions m_options;
    std::vector<SearchResult> m_results;
    int m_currentIndex = -1;

    ResultsChangedCallback m_onResultsChanged;
};

} // namespace docvision
