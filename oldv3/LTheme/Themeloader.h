#ifndef THEMELOADER_H
#define THEMELOADER_H

#include <QString>
#include <QStringList>

class ThemeLoader {
public:
    static void initialize();
    static void applyTheme();
    static void setSelectedTheme(const QString &themeName);
    static QString getSelectedTheme();
    static QStringList getAvailableThemes();
};

#endif // THEMELOADER_H
