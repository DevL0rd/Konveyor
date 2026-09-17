#pragma once

#include <QString>

#include <expected>
#include <optional>

namespace Konveyor::Settings
{

std::optional<QString> readConfigText(const QString &path);
std::expected<void, QString> writeConfigText(const QString &path, const QString &text);

}
