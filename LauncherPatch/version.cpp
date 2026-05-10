#include <QString>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QtGlobal>
#include <QSysInfo>

/**
 * version.cpp - Handles dynamic version detection from patch history.
 */

// External logger interface
void LogLauncherEvent(const QString &message);

int GetBuildNumber() {
    int maxBuild = -1;
    QDirIterator it(":/patches", QStringList() << "*.txt", QDir::Files);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        int currentBuild = -1;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("Build_number")) {
                currentBuild = line.section('=', 1).trimmed().toInt();
            }
        }
        if (currentBuild > maxBuild) {
            maxBuild = currentBuild;
        }
        file.close();
    }
    return maxBuild;
}

QString GetBuildTypePrefix() {
    int maxBuild = -1;
    QString prefix = "release"; // Default

    QDirIterator it(":/patches", QStringList() << "*.txt", QDir::Files);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        int currentBuild = -1;
        QString typePrefix;
        QString buildPrefix;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            int splitIdx = line.indexOf('=');
            if (splitIdx != -1) {
                QString key = line.left(splitIdx).trimmed();
                QString value = line.mid(splitIdx + 1).trimmed();

                if (key == "Build_type_prefix") typePrefix = value;
                else if (key == "Build_prefix") buildPrefix = value;
                else if (key == "Build_number") currentBuild = value.toInt();
            }
        }
        
        if (currentBuild > maxBuild) {
            maxBuild = currentBuild;
            QString found = !typePrefix.isEmpty() ? typePrefix : buildPrefix;
            if (!found.isEmpty()) prefix = found;
        }
        file.close();
    }
    if (prefix.isEmpty()) prefix = "release";
    return prefix;
}

QString GetLauncherTitle() {
    int maxBuild = -1;
    QMap<QString, QString> bestPatchData;

    // Search in the internal program resources (compiled in)
    QDirIterator it(":/patches", QStringList() << "*.txt", QDir::Files);

    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QTextStream in(&file);
        QMap<QString, QString> currentData;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            
            int splitIdx = line.indexOf('=');
            if (splitIdx != -1) {
                QString key = line.left(splitIdx).trimmed();
                QString value = line.mid(splitIdx + 1).trimmed();
                currentData[key] = value;

                // If value starts a bracket block, read until the end bracket
                if (value.startsWith("[")) {
                    QString block = value;
                    while (!block.contains(']') && !in.atEnd()) {
                        QString nextLine = in.readLine();
                        block += "\n" + nextLine;
                    }
                    currentData[key] = block;
                }
            } else if (line.startsWith("set ")) {
                // Handle Build Legit Mapping: set "undev" = UnderDevelopment
                // Syntax: set "key" = value
                int firstQuote = line.indexOf('"');
                int secondQuote = line.indexOf('"', firstQuote + 1);
                int eqIdx = line.indexOf('=', secondQuote + 1);

                if (firstQuote != -1 && secondQuote != -1 && eqIdx != -1) {
                    QString key = line.mid(firstQuote + 1, secondQuote - firstQuote - 1);
                    QString value = line.mid(eqIdx + 1).trimmed();
                    // We store these in the map using a "mapping_" prefix
                    currentData["mapping_" + key] = value;
                }
            }
        }
        file.close();
        
        // Search for the highest Build_number to find the newest version
        bool ok;
        int buildNum = currentData.value("Build_number").toInt(&ok);
        if (ok && buildNum > maxBuild) {
            maxBuild = buildNum;
            bestPatchData = currentData;
        }
    }

    if (maxBuild == -1 || bestPatchData.isEmpty()) {
        return QString("%1 %2").arg(LAUNCHER_APP_NAME, LAUNCHER_VERSION);
    }

    // 1. Get template based on Platform (Windows vs Linux)
    QString title;
#ifdef Q_OS_WIN
    title = bestPatchData.value("Core_WindowTitle_Windows", "<App_name> <App_ver> (Windows)");
#else
    title = bestPatchData.value("Core_WindowTitle_Linux", "<App_name> <App_ver> (LinuxQt)");
#endif

    // Resolve Build Legit Prefix Mapping
    QString bVerPrefix = bestPatchData.value("Build_prefix", "000");
    QString btPrefix = bestPatchData.value("Build_type_prefix", "release");

    // Determine the full Build Type name
    QString fullType = bestPatchData.value("Build_type");
    if (fullType.isEmpty()) {
        // Fallback to the 'set' mapping or the prefix itself
        fullType = bestPatchData.value("mapping_" + btPrefix, btPrefix);
    }

    // 2. Replace tokens from the patch data
    title.replace("<App_name>", bestPatchData.value("App_name", LAUNCHER_APP_NAME));
    title.replace("<App_ver>", bestPatchData.value("App_ver", LAUNCHER_VERSION));
    title.replace("<Build_prefix>", bVerPrefix);
    title.replace("<Build_type_prefix>", btPrefix);
    title.replace("<Build_type>", fullType);

    LogLauncherEvent(QString("Version Detected: %1 (Build %2) for %3")
                     .arg(bestPatchData.value("App_ver", "0.0"), 
                          bestPatchData.value("Build_number", "0"), 
                          QSysInfo::productType()));

    return title;
}

QString GetLatestUpdateNote() {
    int maxBuild = -1;
    QMap<QString, QString> bestPatchData;

    QDirIterator it(":/patches", QStringList() << "*.txt", QDir::Files);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        QMap<QString, QString> currentData;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            int splitIdx = line.indexOf('=');
            if (splitIdx != -1) {
                QString key = line.left(splitIdx).trimmed();
                QString value = line.mid(splitIdx + 1).trimmed();
                if (value.startsWith("[")) {
                    QString block = value;
                    while (!block.contains(']') && !in.atEnd()) {
                        block += "\n" + in.readLine();
                    }
                    currentData[key] = block;
                } else {
                    currentData[key] = value;
                }
            }
        }
        file.close();
        
        bool ok;
        int buildNum = currentData.value("Build_number").toInt(&ok);
        if (ok && buildNum > maxBuild) {
            maxBuild = buildNum;
            bestPatchData = currentData;
        }
    }

    QString rawNote = bestPatchData.value("Update_note", "No update notes available for this build.");

    return rawNote.trimmed();
}

QString GetAppName() {
    int maxBuild = -1;
    QString appName = LAUNCHER_APP_NAME;

    QDirIterator it(":/patches", QStringList() << "*.txt", QDir::Files);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QTextStream in(&file);
        int currentBuild = -1;
        QString currentName;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            int splitIdx = line.indexOf('=');
            if (splitIdx != -1) {
                QString key = line.left(splitIdx).trimmed();
                QString value = line.mid(splitIdx + 1).trimmed();
                if (key == "Build_number") currentBuild = value.toInt();
                else if (key == "App_name") currentName = value;
            }
        }
        file.close();

        if (currentBuild > maxBuild) {
            maxBuild = currentBuild;
            if (!currentName.isEmpty()) appName = currentName;
        }
    }

    return appName;
}

QString GetMadeChangesTitle() {
    int maxBuild = -1;
    QMap<QString, QString> bestPatchData;

    QDirIterator it(":/patches", QStringList() << "*.txt", QDir::Files);

    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QTextStream in(&file);
        QMap<QString, QString> currentData;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            int splitIdx = line.indexOf('=');
            if (splitIdx != -1) {
                QString key = line.left(splitIdx).trimmed();
                QString value = line.mid(splitIdx + 1).trimmed();
                currentData[key] = value;

                if (value.startsWith("[")) {
                    QString block = value;
                    while (!block.contains(']') && !in.atEnd()) {
                        QString nextLine = in.readLine();
                        block += "\n" + nextLine;
                    }
                    currentData[key] = block;
                }
            }
        }
        file.close();

        bool ok;
        int buildNum = currentData.value("Build_number").toInt(&ok);
        if (ok && buildNum > maxBuild) {
            maxBuild = buildNum;
            bestPatchData = currentData;
        }
    }

    if (maxBuild == -1 || bestPatchData.isEmpty()) {
        return "RJ Launcher Maintenance";
    }

    QString title = bestPatchData.value("MadeChangesTools", "0.<App_ver>.<Build_prefix>.<Build_number>");

    // Resolve tokens
    QString bVerPrefix = bestPatchData.value("Build_prefix", "000");
    QString btPrefix = bestPatchData.value("Build_type_prefix", "release");

    title.replace("<App_name>", bestPatchData.value("App_name", LAUNCHER_APP_NAME));
    title.replace("<App_ver>", bestPatchData.value("App_ver", LAUNCHER_VERSION));
    title.replace("<Build_prefix>", bVerPrefix);
    title.replace("<Build_type_prefix>", btPrefix);
    title.replace("<Build_type>", bestPatchData.value("Build_type", btPrefix));
    title.replace("<Build_number>", QString::number(maxBuild));

    if (!title.startsWith("MadeChanges")) title = "MadeChanges " + title;
    return title;
}