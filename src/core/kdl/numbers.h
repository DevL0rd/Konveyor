#pragma once

#include <QString>

#include <expected>
#include <string_view>
#include <variant>

namespace Konveyor::Kdl
{

struct NumberError
{
    qsizetype offset = 0;
    QString message;
};

using NumberValue = std::variant<qint64, double>;

bool looksLikeNumber(std::u32string_view token);
std::expected<NumberValue, NumberError> parseNumber(std::u32string_view token);

}
