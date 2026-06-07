#ifndef VERSION_H
#define VERSION_H

#include <QString>

namespace AppVersion {
// Declare getters
QString appVer();
QString appBuildNumber();
QString appBuild();
QString type();
QString buildPackages();
QString full();
}

#endif // VERSION_H
