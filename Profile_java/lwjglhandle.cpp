#include <QString>

/**
 * lgwl-handlelib.cpp - Handles LWJGL version detection for specific Minecraft versions.
 */

QString GetLwglVersionForMc(const QString &mcVersion) {
    // Mapping Minecraft versions to recommended LWJGL versions

    if (mcVersion.contains("26.") || mcVersion.startsWith("1.26")) {
        return "3.4.1";
    }

    if (mcVersion.startsWith("1.21") || mcVersion.startsWith("1.22")) {
        return "3.3.3";
    }

    if (mcVersion.startsWith("1.19") || mcVersion.startsWith("1.20")) {
        return "3.3.1";
    }

    if (mcVersion.startsWith("1.13") || mcVersion.startsWith("1.14") || 
        mcVersion.startsWith("1.15") || mcVersion.startsWith("1.16") || 
        mcVersion.startsWith("1.17") || mcVersion.startsWith("1.18")) {
        return "3.2.2";
    }

    // Default fallback for modern Minecraft versions
    return "3.3.1";
}