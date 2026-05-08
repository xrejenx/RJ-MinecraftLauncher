#include <QFile>
#include <QTextStream>
#include <QString>

QString GetUpdateRepositoryUrl() {
    QFile file(":/config/CMakeSplashScreen/api.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("api =")) {
                return "https://api.github.com/repos/" + line.section('=', 1).trimmed();
            }
        }
    }
    // Fallback if resource is missing
    return "https://api.github.com/repos/xrejenx/RJ-MinecraftLauncher";
}