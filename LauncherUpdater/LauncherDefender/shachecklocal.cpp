#include <QFile>
#include <QCryptographicHash>
#include <QString>
#include <QDir>

bool VerifyLocalZip(const QString &filePath, const QString &expectedHash, QCryptographicHash::Algorithm algo) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QCryptographicHash hash(algo);
    if (hash.addData(&file)) {
        QString result = hash.result().toHex();
        if (result.compare(expectedHash, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    
    // If integrity fails, we delete the package to trigger a fresh download
    file.close();
    QFile::remove(filePath);
    return false;
}