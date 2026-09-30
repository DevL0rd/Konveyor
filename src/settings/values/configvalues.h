#pragma once

#include "config/types.h"

#include <QString>
#include <QVariantMap>

namespace Konveyor::Settings
{

QVariantMap globalValues(const Config::Config &config);
QVariantMap layoutValues(const Config::Layout &layout);
QVariant colorValue(const QColor &color);
QVariant paintValue(const std::optional<Config::Paint> &paint);
QVariant sizeValue(const std::optional<Config::PresetSize> &size);

}
