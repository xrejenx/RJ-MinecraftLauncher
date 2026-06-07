#include <QString>
#include <QPixmap>

extern QString GetBuildTypePrefix();

namespace SplashAssetManager {
    QPixmap getSplashLogo() {
        QString prefix = GetBuildTypePrefix().toLower();
        // Using CmakeLauncherIcon source for splash as requested
        QString path = QString(":/launcher/icons/CmakeLauncherIcon/%1/icon.png").arg(prefix);
        QPixmap pix(path);
        return pix;
    }
}