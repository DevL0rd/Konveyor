#pragma once

#include "layout/common/layouttypes.h"

#include <QHash>
#include <QJsonObject>
#include <QPointF>
#include <QSize>
#include <QString>

#include <optional>

namespace Konveyor::Layout
{

struct RememberedWindow
{
    std::optional<ColumnWidth> columnWidth;
    std::optional<QSize> floatingSize;
    std::optional<QPointF> floatingPosition;
    bool operator==(const RememberedWindow &) const = default;
};

using WindowMemory = QHash<QString, RememberedWindow>;

QJsonObject windowMemoryToJson(const WindowMemory &memory);
WindowMemory windowMemoryFromJson(const QJsonObject &json);

}
