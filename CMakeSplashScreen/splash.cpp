#include "splash.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

// Use forward declarations instead of including .cpp files to avoid linker errors
QString GetLauncherTitle();

SplashScreen::SplashScreen(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    setWindowTitle(GetLauncherTitle());
    setFixedSize(450, 300);
    setupUi();
}

void SplashScreen::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 20, 40, 20); // Add margins to replace the previous QSS margin

    logoLabel = new QLabel(this);
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel, 1);

    // Create the "Glow" effect
    QGraphicsDropShadowEffect *glow = new QGraphicsDropShadowEffect(this);
    glow->setBlurRadius(20);
    glow->setColor(QColor("#90EE90")); // Light Green glow
    glow->setOffset(0, 0);
    logoLabel->setGraphicsEffect(glow);

    // Pulse Animation for the glow
    QPropertyAnimation *pulse = new QPropertyAnimation(glow, "blurRadius", this);
    pulse->setDuration(1500);
    pulse->setStartValue(10);
    pulse->setEndValue(50);
    pulse->setEasingCurve(QEasingCurve::InOutQuad);
    pulse->setLoopCount(-1); // Infinite loop
    pulse->start();

    progressBar = new QProgressBar(this);
    // Removing style sheets allows the OS to render its own native progress bar
    progressBar->setTextVisible(false); 
    mainLayout->addWidget(progressBar);

    taskLabel = new QLabel("Initializing...", this);
    mainLayout->addWidget(taskLabel);
}

void SplashScreen::setTaskText(const QString &text) {
    taskLabel->setText(text);
}

void SplashScreen::setProgress(int value) {
    progressBar->setValue(value);
}

void SplashScreen::setLogo(const QPixmap &pixmap) {
    logoLabel->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void SplashScreen::setBgColor(const QColor &color) {
    QPalette pal = palette();
    pal.setColor(QPalette::Window, color);
    pal.setColor(QPalette::WindowText, Qt::white); // Ensure text is readable
    setPalette(pal);
    setAutoFillBackground(true);
    
    // Set global text color for labels inside the splash
    setStyleSheet("QLabel { color: white; }");
}