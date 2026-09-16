#include "layout/engine/windowmemory.h"

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
    return object;
}

RememberedWindow entryFromJson(const QJsonObject &object)
{
    RememberedWindow entry;
    const QJsonObject width = object[QStringLiteral("column-width")].toObject();
    if (width.contains(QStringLiteral("value"))) {
        const double value = width[QStringLiteral("value")].toDouble();
        entry.columnWidth = width[QStringLiteral("proportion")].toBool() ? ColumnWidth::proportion(value) : ColumnWidth::fixed(value);
    }
    const QJsonArray size = object[QStringLiteral("floating-size")].toArray();
    if (size.size() == 2) {
        entry.floatingSize = QSize(size[0].toInt(), size[1].toInt());
    }
    const QJsonArray position = object[QStringLiteral("floating-position")].toArray();
    if (position.size() == 2) {
        entry.floatingPosition = QPointF(position[0].toDouble(), position[1].toDouble());
    }
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
