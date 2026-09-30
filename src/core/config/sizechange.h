#pragma once

#include "config/types.h"

#include <QString>

#include <expected>

namespace Konveyor::Config
{

std::expected<SizeChange, QString> parseSizeChange(const QString &text, bool fixedIsInteger);

}
