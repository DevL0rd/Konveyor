#pragma once

#include "config/types.h"

#include <QString>
#include <QVariantList>

namespace Konveyor::Settings
{

QString profileNameFor(const Config::Config &config, const QVariantMap &output);
QVariantList windowsMatchingRule(const Config::Config &config, const Config::WindowRule &rule, const QVariantList &windows,
    const QVariantList &workspaces, const QVariantList &outputs);

}
