#include "layout/common/sizechange.h"

#include "config/sizechange.h"

namespace Konveyor::Layout
{

std::expected<SizeChange, QString> parseSizeChange(const QString &text)
{
    return Config::parseSizeChange(text, true).transform([](Config::SizeChange change) { return SizeChange {change.kind, change.value}; });
}

std::expected<PositionChange, QString> parsePositionChange(const QString &text)
{
    return Config::parseSizeChange(text, false).transform([](Config::SizeChange change) {
        return PositionChange {change.kind, change.value};
    });
}

std::expected<Config::WorkspaceReference, QString> parseWorkspaceReference(const QString &text)
{
    Config::WorkspaceReference reference;
    bool ok = false;
    const uint index = text.toUInt(&ok);
    if (ok) {
        if (index > 255) {
            return std::unexpected(QStringLiteral("workspace index out of range: %1").arg(text));
        }
        reference.kind = Config::WorkspaceReferenceKind::Index;
        reference.index = index;
        return reference;
    }
    if (text.isEmpty()) {
        return std::unexpected(QStringLiteral("workspace reference is missing"));
    }
    reference.kind = Config::WorkspaceReferenceKind::Name;
    reference.name = text;
    return reference;
}

std::expected<Config::ColumnDisplay, QString> parseColumnDisplay(const QString &text)
{
    if (text == QLatin1String("normal")) {
        return Config::ColumnDisplay::Normal;
    }
    if (text == QLatin1String("tabbed")) {
        return Config::ColumnDisplay::Tabbed;
    }
    return std::unexpected(QStringLiteral(R"(invalid column display, can be "normal" or "tabbed")"));
}

std::expected<bool, QString> parseBool(const QString &text)
{
    if (text == QLatin1String("true") || text == QLatin1String("#true")) {
        return true;
    }
    if (text == QLatin1String("false") || text == QLatin1String("#false")) {
        return false;
    }
    return std::unexpected(QStringLiteral("invalid boolean: %1").arg(text));
}

std::expected<quint64, QString> parseIndex(const QString &text)
{
    bool ok = false;
    const quint64 value = text.toULongLong(&ok);
    if (!ok) {
        return std::unexpected(QStringLiteral("invalid index: %1").arg(text));
    }
    return value;
}

}
