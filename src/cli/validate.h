#pragma once

#include <QString>
#include <QTextStream>

namespace Konveyor::Cli
{

int runValidate(const QString &path, QTextStream &out, QTextStream &err);

}
