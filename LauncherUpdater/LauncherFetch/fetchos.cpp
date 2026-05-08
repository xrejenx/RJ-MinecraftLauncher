#include <QtGlobal>
#include <QString>

namespace FetchOS {
    QString getPlatformName() {
#ifdef Q_OS_WIN
        return "windows";
#else
        return "linux";
#endif
    }
}