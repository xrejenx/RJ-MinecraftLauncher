#include <QFile>
#include <QCryptographicHash>

bool VerifySHA256(const QString &filePath, const QString &expectedHash) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (hash.addData(&file)) {
        return QString::fromLatin1(hash.result().toHex()).compare(expectedHash, Qt::CaseInsensitive) == 0;
    }
    return false;
}