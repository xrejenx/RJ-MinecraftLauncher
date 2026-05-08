#include "splash.h"
#include "LauncherPatch/version.cpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

SplashScreen::SplashScreen(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setFixedSize(450, 300);
    setupUi();
}

void SplashScreen::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 10);

    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(30);
    titleBar->setStyleSheet("background-color: rgba(0,0,0,50);");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 5, 0);

    titleLayout->addWidget(new QLabel("[RJ]", this));
    titleLabel = new QLabel(GetLauncherTitle(), this);
    titleLabel->setStyleSheet("font-size: 10px; font-weight: bold; color: black;");
    titleLayout->addWidget(titleLabel, 1);
    
    QPushButton *closeBtn = new QPushButton("X", this);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet("background: transparent; border: none; font-weight: bold; color: black;");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(closeBtn);
    mainLayout->addWidget(titleBar);

    logoLabel = new QLabel(this);
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel, 1);

    progressBar = new QProgressBar(this);
    progressBar->setFixedHeight(15);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%p%");
    progressBar->setStyleSheet("QProgressBar { border: 1px solid grey; border-radius: 5px; text-align: right; margin: 0 40px; } "
                               "QProgressBar::chunk { background-color: #05B8CC; }");
    mainLayout->addWidget(progressBar);

    taskLabel = new QLabel("Initializing...", this);
    taskLabel->setStyleSheet("font-size: 10px; padding-left: 10px; color: black;");
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
    setPalette(pal);
    setAutoFillBackground(true);
}