#ifndef DOWNLOADZIP_H
#define DOWNLOADZIP_H

#include <QString>

bool ExtractZipFile(const QString &zipPath, const QString &extractDir);
bool ProcessZipUpdate(const QString &url, const QString &version);

#endif // DOWNLOADZIP_H