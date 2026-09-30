#include "config/sizechange.h"

namespace Konveyor::Config
{

namespace
{

std::expected<double, QString> parseNumber(const QString &text, bool integer)
{
    bool ok = false;
    const double value = integer ? static_cast<double>(text.toInt(&ok)) : text.toDouble(&ok);
    if (!ok) {
        return std::unexpected(QStringLiteral("error parsing value"));
    }
    return value;
}

}

std::expected<SizeChange, QString> parseSizeChange(const QString &text, bool fixedIsInteger)
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
        return SizeChange {adjust ? SizeChangeKind::AdjustProportion : SizeChangeKind::SetProportion, *number};
    }
    return SizeChange {adjust ? SizeChangeKind::AdjustFixed : SizeChangeKind::SetFixed, *number};
}

}
