#pragma once

#include <QKeySequence>
#include <QString>

namespace Konveyor::Settings
{

QString bindKeyName(const QKeySequence &sequence, const QString &modKey);

}
