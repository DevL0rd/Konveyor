#pragma once

#include <QString>

#include <expected>

namespace Konveyor::Config
{

std::expected<QString, QString> setForceResizableRule(const QString &text, const QString &fileName, const QString &appId, bool enabled);
std::expected<QString, QString> ensureTrailingForceResizableInclude(const QString &text, const QString &fileName);

}
