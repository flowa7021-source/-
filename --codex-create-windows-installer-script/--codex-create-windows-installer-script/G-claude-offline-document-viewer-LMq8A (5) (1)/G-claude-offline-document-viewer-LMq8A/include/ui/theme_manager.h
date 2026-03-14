#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace docvision {

enum class Theme {
    Light,
    Dark,
    NightInversion     // inverted colors for comfortable night reading
};

struct ThemeColors {
    COLORREF background;
    COLORREF foreground;
    COLORREF accent;
    COLORREF ribbonBg;
    COLORREF tabBarBg;
    COLORREF tabActive;
    COLORREF tabInactive;
    COLORREF sidePanelBg;
    COLORREF statusBarBg;
    COLORREF buttonHover;
    COLORREF buttonPressed;
    COLORREF selection;
    COLORREF scrollbar;
    COLORREF border;
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF homeScreenBg;
    COLORREF cardBg;
    COLORREF cardHover;
};

class ThemeManager {
public:
    ThemeManager();

    void setTheme(Theme theme);
    Theme getTheme() const { return m_theme; }
    const ThemeColors& getColors() const { return m_colors; }

    // Convenience
    HBRUSH getBgBrush() const { return m_bgBrush; }
    HFONT getFont(int size = 0) const;
    HFONT getBoldFont(int size = 0) const;

    // Change notification
    using ThemeChangedCallback = std::function<void(Theme)>;
    void onThemeChanged(ThemeChangedCallback cb) { m_onChanged = cb; }

    // DPI-aware font sizes
    void onDPIChanged(UINT dpi);

private:
    void applyTheme();
    void createFonts();

    Theme m_theme = Theme::Light;
    ThemeColors m_colors{};
    HBRUSH m_bgBrush = nullptr;
    HFONT m_normalFont = nullptr;
    HFONT m_boldFont = nullptr;
    UINT m_dpi = 96;
    ThemeChangedCallback m_onChanged;
};

} // namespace docvision
