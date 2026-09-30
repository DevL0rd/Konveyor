#pragma once

#include "config/types.h"

#include <QString>
#include <QVariantList>

namespace Konveyor::Settings
{

QString profileNameFor(const Config::Config &config, const QVariantMap &output);
Config::Layout scopedLayout(
    const Config::Config &config, const QString &kind, const QString &name, const QVariantList &outputs, const QVariantList &workspaces);
QVariantList windowsMatchingRule(const Config::Config &config, const Config::WindowRule &rule, const QVariantList &windows,
    const QVariantList &workspaces, const QVariantList &outputs);

}
