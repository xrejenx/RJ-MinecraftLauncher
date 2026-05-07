#include <QString>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

/**
 * vanilla_installer.cpp - Manages installation prefix and executable jar mapping.
 */

void Insta_prefix_vanillamc_maker(const QString &path, const QString &version) {
    QString configPath = path + "/install_manifest.json";
    QFile file(configPath);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["prefix"] = "vanilla-mc";
        obj["version"] = version;
        obj["executable_jar"] = QString("Minecraft %1.jar").arg(version);
        obj["lib_path"] = "Lib";
        
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}