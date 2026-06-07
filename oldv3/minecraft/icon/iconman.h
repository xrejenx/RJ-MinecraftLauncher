#ifndef ICONMAN_H
#define ICONMAN_H

#include <QDialog>
#include <QString>
#include <QListWidget>
#include <QLabel>
#include <QList>

class IconManager : public QDialog {
    Q_OBJECT
public:
    explicit IconManager(QWidget *parent = nullptr);
    QString getSelectedIconPath() const;
signals:
    void iconSelected(const QString &iconPath); // Example signal

private:
    void scanIcons();
    void populateList();

    QListWidget *iconList;
    QLabel *totalLabel;
    QString m_selectedPath;

    struct IconSet {
        QString name;
        QString pngPath;
        QString icoPath;
    };
    QList<IconSet> m_iconSets;
};

#endif // ICONMAN_H