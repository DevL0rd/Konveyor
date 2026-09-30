#pragma once

#include <QString>
#include <QStringList>

namespace Konveyor::Settings
{

QStringList includesOverriding(const QString &mainText, const QString &configPath, const QString &path);

}
