#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QApplication>
#include <QStringList>
#include <functional>

struct CustomThemeInfo {
    QString name;
    QString imagePath;
    QString color; // Hex
    bool autoDetect;
};

class ThemeLoader {
public:
    static void initialize();
    static void applyTheme();
    static void setSelectedTheme(const QString &themeName);
    static QString getSelectedTheme();
    static QStringList getAvailableThemes();
    static void createCustomTheme(const CustomThemeInfo &info);
    static QString getThemeImagePath(const QString &themeName);

private:
    static void createDefaultThemes();
};

void ShowCreateThemeDialog(QWidget *parent, const std::function<void()> &onCreated);

#endif // THEME_H
