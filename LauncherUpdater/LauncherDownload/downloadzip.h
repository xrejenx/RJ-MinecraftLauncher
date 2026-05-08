#ifndef DOWNLOADZIP_H
#define DOWNLOADZIP_H

#include <QString>

bool ExtractZipFile(const QString &zipFilePath, const QString &destinationPath);
bool ProcessZipUpdate(const QString &url, const QString &version);

#endif // DOWNLOADZIP_H