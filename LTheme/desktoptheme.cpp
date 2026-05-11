#include "desktoptheme.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QPalette>

bool isSystemDarkMode() {
    // Prioritize Qt 6.5+ native color scheme detection
    auto hints = QGuiApplication::styleHints();
    if (hints && hints->colorScheme() != Qt::ColorScheme::Unknown) {
        return hints->colorScheme() == Qt::ColorScheme::Dark;
    }

    // Fallback detection: Check if WindowText is lighter than the Window background
    // This reliably indicates a dark-themed environment in older Qt versions.
    QPalette pal = QGuiApplication::palette();
    return pal.color(QPalette::WindowText).lightness() > pal.color(QPalette::Window).lightness();
}

QString getAutoThemeName() {
    // Normalize theme names for internal consistency
    return isSystemDarkMode() ? "Ddark" : "Llight";
}