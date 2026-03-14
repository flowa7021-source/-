#pragma once

#include <vector>
#include <functional>

namespace docvision {

// Navigation position
struct NavigationPosition {
    int pageIndex = 0;
    double scrollX = 0;
    double scrollY = 0;
    double zoom = 1.0;
};

// Navigation history — back/forward stack
class NavigationHistory {
public:
    NavigationHistory() = default;

    void push(const NavigationPosition& pos);
    bool canGoBack() const;
    bool canGoForward() const;
    NavigationPosition goBack();
    NavigationPosition goForward();
    NavigationPosition current() const;
    void clear();

    size_t size() const { return m_history.size(); }
    size_t currentIndex() const { return m_currentIndex; }

private:
    std::vector<NavigationPosition> m_history;
    size_t m_currentIndex = 0;
    static constexpr size_t MAX_HISTORY = 100;
};

} // namespace docvision
