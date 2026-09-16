#include "layout/common/sizechange.h"

namespace Konveyor::Layout
{

namespace
{

struct ParsedChange
{
    ChangeKind kind;
    double value;
};

std::expected<double, QString> parseNumber(const QString &text, bool integer)
{
    bool ok = false;
    const double value = integer ? static_cast<double>(text.toInt(&ok)) : text.toDouble(&ok);
    if (!ok) {
        return std::unexpected(QStringLiteral("error parsing value"));
    }
    return value;
}

std::expected<ParsedChange, QString> parseChange(const QString &text, bool fixedIsInteger)
{
    const qsizetype percent = text.indexOf(QLatin1Char('%'));
    const bool isProportion = percent >= 0;
    if (isProportion && percent != text.size() - 1) {
        return std::unexpected(QStringLiteral("trailing characters after '%' are not allowed"));
    }
    const QString value = isProportion ? text.left(percent) : text;
    if (value.isEmpty()) {
        return std::unexpected(QStringLiteral("value is missing"));
    }
    const bool adjust = value.front() == QLatin1Char('+') || value.front() == QLatin1Char('-');
    const auto number = parseNumber(value, fixedIsInteger && !isProportion);
    if (!number) {
        return std::unexpected(number.error());
    }
    if (isProportion) {
        return ParsedChange {adjust ? ChangeKind::AdjustProportion : ChangeKind::SetProportion, *number};
    }
    return ParsedChange {adjust ? ChangeKind::AdjustFixed : ChangeKind::SetFixed, *number};
}

}

std::expected<SizeChange, QString> parseSizeChange(const QString &text)
{
    return parseChange(text, true).transform([](ParsedChange change) { return SizeChange {change.kind, change.value}; });
}

std::expected<PositionChange, QString> parsePositionChange(const QString &text)
{
    return parseChange(text, false).transform([](ParsedChange change) { return PositionChange {change.kind, change.value}; });
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
