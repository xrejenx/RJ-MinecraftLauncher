#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QMap>

QColor GetLightestDominantColor(const QPixmap &pixmap) {
    if (pixmap.isNull()) return Qt::white;
    QImage img = pixmap.toImage();
    QMap<QRgb, int> counts;
    
    for(int y=0; y<img.height(); ++y) {
        for(int x=0; x<img.width(); ++x) {
            QRgb pix = img.pixel(x,y);
            if (qAlpha(pix) > 200) counts[pix]++;
        }
    }

    QRgb lightest = qRgb(255,255,255);
    int maxCount = 0;
    for(auto it = counts.begin(); it != counts.end(); ++it) {
        QColor col(it.key());
        // Filter for lightest but dominant
        if (col.lightness() > 180 && it.value() > maxCount) {
            maxCount = it.value();
            lightest = it.key();
        }
    }
    return QColor(lightest);
}