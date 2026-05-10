#include "desktoptheme.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QPalette>

bool isSystemDarkMode() {
    // Qt 6.5+ native detection
    if (auto hints = QGuiApplication::styleHints()) {
        return hints->colorScheme() == Qt::ColorScheme::Dark;
    }
    
    // Fallback for older Qt6/Environmental edge cases
    QPalette pal = QGuiApplication::palette();
    return pal.color(QPalette::WindowText).lightness() > pal.color(QPalette::Window).lightness();
}

QString getAutoThemeName() {
    return isSystemDarkMode() ? "DDark" : "LLight";
}