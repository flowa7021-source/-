#include "core/navigation_history.h"

namespace docvision {

void NavigationHistory::push(const NavigationPosition& pos) {
    // Remove forward history
    if (m_currentIndex + 1 < m_history.size()) {
        m_history.erase(m_history.begin() + m_currentIndex + 1, m_history.end());
    }

    m_history.push_back(pos);

    // Trim if too large
    if (m_history.size() > MAX_HISTORY) {
        m_history.erase(m_history.begin());
    }

    m_currentIndex = m_history.size() - 1;
}

bool NavigationHistory::canGoBack() const {
    return m_currentIndex > 0;
}

bool NavigationHistory::canGoForward() const {
    return m_currentIndex + 1 < m_history.size();
}

NavigationPosition NavigationHistory::goBack() {
    if (canGoBack()) {
        --m_currentIndex;
    }
    return current();
}

NavigationPosition NavigationHistory::goForward() {
    if (canGoForward()) {
        ++m_currentIndex;
    }
    return current();
}

NavigationPosition NavigationHistory::current() const {
    if (m_history.empty()) return {};
    return m_history[m_currentIndex];
}

void NavigationHistory::clear() {
    m_history.clear();
    m_currentIndex = 0;
}

} // namespace docvision
