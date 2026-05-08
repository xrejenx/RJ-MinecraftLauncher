#include <QImage>
#include <QColor>
#include <QMap>

QColor GetLightestDominantColor(const QPixmap &pixmap) {
    if (pixmap.isNull()) return Qt::white;
    
    QImage img = pixmap.toImage();
    QMap<QRgb, int> colorCount;
    
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QRgb pixel = img.pixel(x, y);
            if (qAlpha(pixel) > 200) { // Only solid pixels
                colorCount[pixel]++;
            }
        }
    }

    QRgb lightest = qRgb(255, 255, 255);
    int maxFreq = 0;
    for (auto it = colorCount.begin(); it != colorCount.end(); ++it) {
        QColor col(it.key());
        if (col.lightness() > 200 && it.value() > maxFreq) {
            maxFreq = it.value();
            lightest = it.key();
        }
    }
    return QColor(lightest);
}