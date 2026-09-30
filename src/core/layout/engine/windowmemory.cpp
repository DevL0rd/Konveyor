#include "layout/engine/windowmemory.h"

#include "layout/common/sizelimits.h"

#include <QJsonArray>

namespace Konveyor::Layout
{

namespace
{

QJsonObject entryToJson(const RememberedWindow &entry)
{
    QJsonObject object;
    if (entry.columnWidth) {
        object[QStringLiteral("column-width")] = QJsonObject {
            {QStringLiteral("proportion"), entry.columnWidth->isProportion},
            {QStringLiteral("value"), entry.columnWidth->value},
        };
    }
    if (entry.floatingSize) {
        object[QStringLiteral("floating-size")] = QJsonArray {entry.floatingSize->width(), entry.floatingSize->height()};
    }
    if (entry.floatingPosition) {
        object[QStringLiteral("floating-position")] = QJsonArray {entry.floatingPosition->x(), entry.floatingPosition->y()};
    }
    if (entry.nativeSize) {
        object[QStringLiteral("native-size")] = QJsonArray {entry.nativeSize->width(), entry.nativeSize->height()};
    }
    return object;
}

std::optional<QSize> sizeFromJson(const QJsonValue &value)
{
    const QJsonArray array = value.toArray();
    if (array.size() != 2 || !array[0].isDouble() || !array[1].isDouble()) {
        return std::nullopt;
    }
    const QSize size(array[0].toInt(), array[1].toInt());
    if (size.width() < 1 || size.height() < 1 || size.width() > MaxPixelSize || size.height() > MaxPixelSize) {
        return std::nullopt;
    }
    return size;
}

std::optional<ColumnWidth> columnWidthFromJson(const QJsonValue &value)
{
    const QJsonObject object = value.toObject();
    const QJsonValue number = object[QStringLiteral("value")];
    if (!number.isDouble()) {
        return std::nullopt;
    }
    const double width = number.toDouble();
    if (object[QStringLiteral("proportion")].toBool()) {
        return width > 0.0 && width <= MaxProportion ? std::optional(ColumnWidth::proportion(width)) : std::nullopt;
    }
    return width >= 1.0 && width <= MaxPixelSize ? std::optional(ColumnWidth::fixed(width)) : std::nullopt;
}

RememberedWindow entryFromJson(const QJsonObject &object)
{
    RememberedWindow entry;
    entry.columnWidth = columnWidthFromJson(object[QStringLiteral("column-width")]);
    entry.floatingSize = sizeFromJson(object[QStringLiteral("floating-size")]);
    const QJsonArray position = object[QStringLiteral("floating-position")].toArray();
    if (position.size() == 2 && position[0].isDouble() && position[1].isDouble()) {
        entry.floatingPosition = QPointF(position[0].toDouble(), position[1].toDouble());
    }
    entry.nativeSize = sizeFromJson(object[QStringLiteral("native-size")]);
    return entry;
}
}

QJsonObject windowMemoryToJson(const WindowMemory &memory)
{
    QJsonObject json;
    for (auto it = memory.constBegin(); it != memory.constEnd(); ++it) {
        json[it.key()] = entryToJson(it.value());
    }
    return json;
}

WindowMemory windowMemoryFromJson(const QJsonObject &json)
{
    WindowMemory memory;
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        memory.insert(it.key(), entryFromJson(it.value().toObject()));
    }
    return memory;
}

}
