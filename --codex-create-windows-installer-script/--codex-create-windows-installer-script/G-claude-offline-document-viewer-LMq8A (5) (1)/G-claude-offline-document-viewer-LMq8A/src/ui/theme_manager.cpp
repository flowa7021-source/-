#include "ui/theme_manager.h"

namespace docvision {

ThemeManager::ThemeManager() {
    applyTheme();
    createFonts();
}

void ThemeManager::setTheme(Theme theme) {
    m_theme = theme;
    applyTheme();
    if (m_onChanged) m_onChanged(theme);
}

HFONT ThemeManager::getFont(int size) const {
    if (size == 0) return m_normalFont;
    return CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

HFONT ThemeManager::getBoldFont(int size) const {
    if (size == 0) return m_boldFont;
    return CreateFontW(-size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

void ThemeManager::onDPIChanged(UINT dpi) {
    m_dpi = dpi;
    createFonts();
}

void ThemeManager::applyTheme() {
    if (m_bgBrush) DeleteObject(m_bgBrush);

    switch (m_theme) {
        case Theme::Light:
            m_colors = {
                RGB(255, 255, 255),  // background
                RGB(30, 30, 30),     // foreground
                RGB(0, 120, 215),    // accent
                RGB(245, 245, 245),  // ribbonBg
                RGB(240, 240, 240),  // tabBarBg
                RGB(255, 255, 255),  // tabActive
                RGB(230, 230, 230),  // tabInactive
                RGB(250, 250, 250),  // sidePanelBg
                RGB(0, 122, 204),    // statusBarBg
                RGB(229, 241, 251),  // buttonHover
                RGB(204, 228, 247),  // buttonPressed
                RGB(0, 120, 215),    // selection
                RGB(200, 200, 200),  // scrollbar
                RGB(228, 228, 228),  // border
                RGB(30, 30, 30),     // textPrimary
                RGB(120, 120, 120),  // textSecondary
                RGB(245, 247, 250),  // homeScreenBg
                RGB(255, 255, 255),  // cardBg
                RGB(235, 240, 248),  // cardHover
            };
            break;

        case Theme::Dark:
            m_colors = {
                RGB(30, 30, 30),     // background
                RGB(230, 230, 230),  // foreground
                RGB(0, 120, 215),    // accent
                RGB(45, 45, 45),     // ribbonBg
                RGB(40, 40, 40),     // tabBarBg
                RGB(55, 55, 55),     // tabActive
                RGB(35, 35, 35),     // tabInactive
                RGB(37, 37, 37),     // sidePanelBg
                RGB(0, 90, 158),     // statusBarBg
                RGB(62, 62, 62),     // buttonHover
                RGB(75, 75, 75),     // buttonPressed
                RGB(38, 79, 120),    // selection
                RGB(80, 80, 80),     // scrollbar
                RGB(60, 60, 60),     // border
                RGB(230, 230, 230),  // textPrimary
                RGB(160, 160, 160),  // textSecondary
                RGB(25, 25, 25),     // homeScreenBg
                RGB(45, 45, 45),     // cardBg
                RGB(55, 55, 60),     // cardHover
            };
            break;

        case Theme::NightInversion:
            m_colors = {
                RGB(20, 20, 15),     // background (warm black)
                RGB(220, 210, 190),  // foreground (warm white)
                RGB(180, 140, 80),   // accent
                RGB(35, 35, 30),     // ribbonBg
                RGB(30, 30, 25),     // tabBarBg
                RGB(50, 45, 35),     // tabActive
                RGB(25, 25, 20),     // tabInactive
                RGB(30, 30, 25),     // sidePanelBg
                RGB(50, 40, 25),     // statusBarBg
                RGB(55, 50, 40),     // buttonHover
                RGB(65, 60, 45),     // buttonPressed
                RGB(80, 65, 40),     // selection
                RGB(60, 55, 45),     // scrollbar
                RGB(50, 45, 35),     // border
                RGB(220, 210, 190),  // textPrimary
                RGB(150, 140, 120),  // textSecondary
                RGB(18, 18, 14),     // homeScreenBg
                RGB(40, 38, 30),     // cardBg
                RGB(50, 48, 38),     // cardHover
            };
            break;
    }

    m_bgBrush = CreateSolidBrush(m_colors.background);
}

void ThemeManager::createFonts() {
    if (m_normalFont) DeleteObject(m_normalFont);
    if (m_boldFont) DeleteObject(m_boldFont);

    int fontSize = MulDiv(13, m_dpi, 96);
    m_normalFont = CreateFontW(-fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_boldFont = CreateFontW(-fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

} // namespace docvision
