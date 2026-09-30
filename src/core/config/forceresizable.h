#pragma once

#include "kdl/kdl.h"

#include <QString>

#include <expected>

namespace Konveyor::Config
{

std::expected<QString, QString> setForceResizableRule(const QString &text, const QString &fileName, const QString &appId, bool enabled);
bool isForceResizableInclude(const Kdl::Node &node);
std::expected<QString, QString> ensureTrailingForceResizableInclude(const QString &text, const QString &fileName);

}
