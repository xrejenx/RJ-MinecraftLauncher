#include "splash.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPaintEvent>
#include <QPainter>
#include <QRadialGradient>

// Use forward declarations instead of including .cpp files to avoid linker errors
QString GetLauncherTitle();

SplashScreen::SplashScreen(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
    setWindowTitle(GetLauncherTitle());
    setFixedSize(500, 400);
    setupUi();
}

void SplashScreen::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(20);

    mainLayout->addStretch(); // Top stretch for vertical centering

    logoLabel = new QLabel(this);
    logoLabel->setFixedSize(180, 180);
    logoLabel->setScaledContents(false);
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel, 0, Qt::AlignCenter);

    progressBar = new QProgressBar(this);
    progressBar->setTextVisible(false);
    progressBar->setFixedWidth(280);
    progressBar->setFixedHeight(6);
    progressBar->setStyleSheet("QProgressBar { background-color: rgba(0, 0, 0, 25); border: none; border-radius: 3px; } "
                               "QProgressBar::chunk { background-color: #2ecc71; border-radius: 3px; }");
    mainLayout->addWidget(progressBar, 0, Qt::AlignCenter);

    taskLabel = new QLabel("Initializing...", this);
    taskLabel->setAlignment(Qt::AlignCenter);
    taskLabel->setStyleSheet("font-weight: 500; color: #2c3e50; font-size: 12px;");
    mainLayout->addWidget(taskLabel);

    mainLayout->addStretch(); // Bottom stretch for vertical centering
}

void SplashScreen::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event); // Use native OS background rendering
}

void SplashScreen::setTaskText(const QString &text) {
    taskLabel->setText(text);
}

void SplashScreen::setProgress(int value) {
    progressBar->setValue(value);
}

void SplashScreen::setLogo(const QPixmap &pixmap) {
    logoLabel->setPixmap(pixmap.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void SplashScreen::setBgColor(const QColor &color) {
    // Transparency is handled by WA_TranslucentBackground
}