#pragma once

#include <QWidget>
#include <QString>

/**
 * Handles dynamic icon application based on the Build legit prefix.
 */
void ApplyLauncherIcon(QWidget* window);
QString GetBuildTypePrefix();