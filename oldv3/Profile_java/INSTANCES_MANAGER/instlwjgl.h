#ifndef INSTLWJGL_H
#define INSTLWJGL_H

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QWidget>

class InstLWJGL : public QObject {
    Q_OBJECT
public:
    explicit InstLWJGL(QObject *parent = nullptr);
    bool downloadLWJGL(const QVariantMap &metadata, const QString &targetDirPath, QWidget *parentWidget);
signals:
    void lwjglDownloadFinished(bool success);
};

#endif // INSTLWJGL_H