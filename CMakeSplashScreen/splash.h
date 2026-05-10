#pragma once
#include <QDialog>
#include <QLabel>
#include <QProgressBar>

class QPaintEvent;

class SplashScreen : public QDialog {
    Q_OBJECT
public:
    explicit SplashScreen(QWidget *parent = nullptr);
    
    void setTaskText(const QString &text);
    void setProgress(int value);
    void setLogo(const QPixmap &pixmap);
    void setBgColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *titleLabel;
    QLabel *logoLabel;
    QLabel *taskLabel;
    QProgressBar *progressBar;
    
    void setupUi();
};