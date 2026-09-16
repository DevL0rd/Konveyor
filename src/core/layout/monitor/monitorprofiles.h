#pragma once

#include "config/types.h"

#include <QSizeF>
#include <QString>

namespace Konveyor::Layout
{

const Config::MonitorProfile *monitorProfileFor(const Config::Config &config, const QString &name, QSizeF size);

}
