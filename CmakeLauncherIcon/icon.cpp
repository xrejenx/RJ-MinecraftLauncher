#include "icon.h"
#include <QIcon>
#include <QFile>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

// External bridge to version detection
extern QString GetBuildTypePrefix();

void ApplyLauncherIcon(QWidget* window) {
    if (!window) return;

    QString prefix = GetBuildTypePrefix().toLower();
    QString iconPath = QString(":/launcher/icons/CmakeLauncherIcon/%1/icon.png").arg(prefix);

    if (QFile::exists(iconPath)) {
        QPixmap pix(iconPath);
        if (!pix.isNull()) {
            // 1. Auto-Crop: Remove empty transparent space around the logo 
            // to ensure it fills the maximum possible area in the icon slot.
            QImage img = pix.toImage();
            int top = img.height(), bottom = 0, left = img.width(), right = 0;
            bool hasContent = false;

            for (int y = 0; y < img.height(); ++y) {
                for (int x = 0; x < img.width(); ++x) {
                    if (qAlpha(img.pixel(x, y)) > 8) { // Ignore nearly transparent pixels
                        top = qMin(top, y); bottom = qMax(bottom, y);
                        left = qMin(left, x); right = qMax(right, x);
                        hasContent = true;
                    }
                }
            }

            if (hasContent) {
                pix = pix.copy(left, top, (right - left) + 1, (bottom - top) + 1);
            }

            // 2. Fill Stretch: To make small text like "UNDEV" or "BETA" readable,
            // we maximize the height by stretching the logo to fill the square gap.
            // This removes the "tiny" look caused by wide aspect ratios.
            auto createStandardIcon = [&](int s) {
                QPixmap final(s, s);
                final.fill(Qt::transparent);
                QPainter p(&final);
                p.setRenderHint(QPainter::SmoothPixmapTransform);
                
                // Stretch to fill the square, making the version box 50% larger
                // We leave a 1px margin to prevent edge-bleeding on some OS scales
                p.drawPixmap(1, 1, s - 2, s - 2, pix);
                p.end();
                return final;
            };

            // 3. Multi-Size: Generate standard resolutions (16 to 256) so the 
            // Taskbar and Titlebar don't look blurry or "tiny".
            QIcon icon;
            int standardSizes[] = {16, 24, 32, 48, 64, 128, 256};
            for (int s : standardSizes) {
                icon.addPixmap(createStandardIcon(s));
            }
            icon.addPixmap(createStandardIcon(qMax(pix.width(), pix.height())));

            window->setWindowIcon(icon);
            QApplication::setWindowIcon(icon); // Set globally for all dialogs
        }
    } else {
        qDebug() << "[ICON ERROR]: Native icon not found for prefix:" << prefix << "at" << iconPath;
    }
}