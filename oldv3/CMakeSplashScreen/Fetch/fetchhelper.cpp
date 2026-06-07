#include <QString>

namespace FetchHelper {
    // Helper logic to validate version strings during the splash sequence
    bool isVersionHigher(const QString &current, const QString &remote) {
        if (current.isEmpty() || remote.isEmpty()) return false;
        return remote.section('.', 0, 0).toInt() > current.section('.', 0, 0).toInt();
    }
}