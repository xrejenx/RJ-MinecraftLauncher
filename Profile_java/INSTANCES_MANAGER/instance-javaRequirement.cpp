#include <QString>

/**
 * instance-javaRequirement.cpp - Central logic for Minecraft version to Java version mapping.
 * Follows official Mojang requirements with custom overrides for development versions.
 */

int GetRequiredJavaMajorVersion(const QString &versionId, int metadataMajor = 0) {
    // Priority 1: Future / Experimental 26.x versions (User requested Java 25)
    if (versionId.contains("26.") || versionId.startsWith("1.26")) {
        return 25;
    }

    // Priority 2: Use metadata if it's explicitly provided by Mojang (modern versions)
    // Mojang manifests now include "javaVersion" objects for 1.17+
    if (metadataMajor > 0) {
        return metadataMajor;
    }

    // Priority 3: Fallback pattern matching for specific version ranges
    
    // Minecraft 1.20.5 and 1.21+
    if (versionId.startsWith("1.21") || versionId.startsWith("1.22") || versionId.contains("24w")) {
        return 21;
    }

    // Minecraft 1.18 to 1.20.4
    if (versionId.startsWith("1.18") || versionId.startsWith("1.19") || versionId.startsWith("1.20")) {
        return 17;
    }

    // Minecraft 1.17
    if (versionId.startsWith("1.17")) {
        return 16;
    }

    // Minecraft 1.0 to 1.16.x
    return 8;
}