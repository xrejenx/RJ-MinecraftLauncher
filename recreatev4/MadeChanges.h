#ifndef MADECHANGES_H
#define MADECHANGES_H

#include <QDialog>
#include <QString>

namespace Ui { class MadeChangesDialog; }

class MadeChangesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MadeChangesDialog(QWidget *parent = nullptr);
    ~MadeChangesDialog() override;

    void appendLogLine(const QString &line);
    void setUpdatingMode(bool updating);

    // Start a direct download (used for branch zip or direct URL)
    void startDownloadFromUrl(const QString &url);

private slots:
    void on_get7zipButton_clicked();
    void on_installButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::MadeChangesDialog *ui;

    class Impl;
    Impl *d;

    void setStatus(const QString &s);
};

#endif // MADECHANGES_H
