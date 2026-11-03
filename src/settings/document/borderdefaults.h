#pragma once

#include <QString>

#include <expected>

namespace Konveyor::Settings
{

std::expected<QString, QString> migrateBorderDefaults(const QString &text, const QString &fileName, const QString &defaults);
std::expected<QString, QString> migrateFocusEffects(const QString &text, const QString &fileName, const QString &defaults);
std::expected<QString, QString> migrateHeatLayers(const QString &text, const QString &fileName, const QString &defaults);

}
