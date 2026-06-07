#include "version.h"

// Core version parts
QString AppVersion::appVer() { return "0.3.11"; }
QString AppVersion::appBuildNumber() { return "42"; }
QString AppVersion::appBuild() { return "792"; }

// Detect OS automatically
QString AppVersion::type() {
#if defined(Q_OS_WIN)
    return "Windows";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_MACOS)
    return "Mac";
#else
    return "UnknownOS";
#endif
}

// Build packages
QString AppVersion::buildPackages() { return "Qt"; }

// Full compressed version string
QString AppVersion::full() {
    return appVer() + "." + appBuildNumber() + "." + appBuild()
    + " (" + type() + buildPackages() + ")";
}
