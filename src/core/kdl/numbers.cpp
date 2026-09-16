#include "kdl/numbers.h"

#include "kdl/characters.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <string>
#include <system_error>

namespace Konveyor::Kdl
{

namespace
{

using NumberResult = std::expected<NumberValue, NumberError>;
using IndexResult = std::expected<std::size_t, NumberError>;

struct RadixPrefix
{
    char32_t marker;
    int radix;
    const char *name;
};

constexpr std::array<RadixPrefix, 3> radixPrefixes {{{U'x', 16, "hexadecimal"}, {U'o', 8, "octal"}, {U'b', 2, "binary"}}};
constexpr quint64 maxMagnitude = quint64(1) << 63;
constexpr qint64 saturatedExponentLimit = 1000000;

char32_t characterAt(std::u32string_view token, std::size_t index)
{
    return index < token.size() ? token[index] : endOfInput;
}

bool isSign(char32_t c)
{
    return c == U'+' || c == U'-';
}

std::unexpected<NumberError> numberError(std::size_t offset, const QString &message)
{
    return std::unexpected(NumberError {static_cast<qsizetype>(offset), message});
}

qint64 applySign(quint64 magnitude, bool negative)
{
    return static_cast<qint64>(negative ? quint64(0) - magnitude : magnitude);
}

NumberResult integerFromDigits(std::u32string_view token, std::size_t start, int radix, const QString &kind, bool negative)
{
    if (!digitValue(characterAt(token, start), radix)) {
        return numberError(start, QStringLiteral("expected %1 digit").arg(kind));
    }
    const quint64 limit = negative ? maxMagnitude : maxMagnitude - 1;
    const auto base = static_cast<quint64>(radix);
    quint64 magnitude = 0;
    for (std::size_t index = start; index < token.size(); ++index) {
        if (token[index] == U'_') {
            continue;
        }
        const std::optional<int> digit = digitValue(token[index], radix);
        if (!digit) {
            return numberError(index, QStringLiteral("invalid character %1 in %2 number").arg(describeCharacter(token[index]), kind));
        }
        const auto digitMagnitude = static_cast<quint64>(*digit);
        if (magnitude > (limit - digitMagnitude) / base) {
            return numberError(0, QStringLiteral("integer does not fit into a signed 64-bit integer"));
        }
        magnitude = magnitude * base + digitMagnitude;
    }
    return applySign(magnitude, negative);
}

std::size_t skipDecimalDigits(std::u32string_view token, std::size_t index)
{
    while (isDecimalDigit(characterAt(token, index)) || characterAt(token, index) == U'_') {
        ++index;
    }
    return index;
}

IndexResult scanDigitsFrom(std::u32string_view token, std::size_t index, const QString &message)
{
    if (!isDecimalDigit(characterAt(token, index))) {
        return numberError(index, message);
    }
    return skipDecimalDigits(token, index);
}

IndexResult scanFraction(std::u32string_view token, std::size_t index)
{
    if (characterAt(token, index) != U'.') {
        return index;
    }
    return scanDigitsFrom(token, index + 1, QStringLiteral("expected digit after decimal point"));
}

IndexResult scanExponent(std::u32string_view token, std::size_t index)
{
    const char32_t marker = characterAt(token, index);
    if (marker != U'e' && marker != U'E') {
        return index;
    }
    const std::size_t digits = isSign(characterAt(token, index + 1)) ? index + 2 : index + 1;
    return scanDigitsFrom(token, digits, QStringLiteral("expected digit in exponent"));
}

qint64 saturatedExponent(std::string_view ascii)
{
    const std::size_t marker = ascii.find_first_of("eE");
    if (marker == std::string_view::npos) {
        return 0;
    }
    std::string_view digits = ascii.substr(marker + 1);
    const bool negative = digits.starts_with('-');
    if (digits.starts_with('-') || digits.starts_with('+')) {
        digits.remove_prefix(1);
    }
    qint64 exponent = 0;
    for (const char digit : digits) {
        exponent = std::min(exponent * 10 + (digit - '0'), saturatedExponentLimit);
    }
    return negative ? -exponent : exponent;
}

double outOfRangeFloat(std::string_view ascii)
{
    const std::string_view mantissa = ascii.substr(0, ascii.find_first_of("eE"));
    const auto point = static_cast<qint64>(std::min(mantissa.find('.'), mantissa.size()));
    const auto firstSignificant = static_cast<qint64>(std::min(mantissa.find_first_of("123456789"), mantissa.size()));
    const qint64 leadingPower = point - firstSignificant + saturatedExponent(ascii);
    const double magnitude = leadingPower > 0 ? std::numeric_limits<double>::infinity() : 0.0;
    return ascii.starts_with('-') ? -magnitude : magnitude;
}

NumberResult floatFromToken(std::u32string_view token)
{
    std::string ascii;
    ascii.reserve(token.size());
    for (std::size_t index = token.front() == U'+' ? 1 : 0; index < token.size(); ++index) {
        if (token[index] != U'_') {
            ascii.push_back(static_cast<char>(token[index]));
        }
    }
    double value = 0.0;
    const std::from_chars_result result = std::from_chars(ascii.data(), ascii.data() + ascii.size(), value);
    if (result.ec == std::errc::result_out_of_range) {
        return outOfRangeFloat(ascii);
    }
    return value;
}

NumberResult parseDecimal(std::u32string_view token, std::size_t start, bool negative)
{
    const IndexResult end = scanDigitsFrom(token, start, QStringLiteral("expected digit"))
                                .and_then([token](std::size_t index) { return scanFraction(token, index); })
                                .and_then([token](std::size_t index) { return scanExponent(token, index); });
    if (!end) {
        return std::unexpected(end.error());
    }
    if (*end < token.size()) {
        return numberError(*end, QStringLiteral("unexpected character %1 in number").arg(describeCharacter(token[*end])));
    }
    if (token.find_first_of(U".eE") != std::u32string_view::npos) {
        return floatFromToken(token);
    }
    return integerFromDigits(token, start, 10, QStringLiteral("decimal"), negative);
}

}

bool looksLikeNumber(std::u32string_view token)
{
    const std::size_t start = isSign(characterAt(token, 0)) ? 1 : 0;
    return isDecimalDigit(characterAt(token, start));
}

std::expected<NumberValue, NumberError> parseNumber(std::u32string_view token)
{
    const bool negative = characterAt(token, 0) == U'-';
    const std::size_t start = isSign(characterAt(token, 0)) ? 1 : 0;
    if (characterAt(token, start) == U'0') {
        for (const RadixPrefix &prefix : radixPrefixes) {
            if (characterAt(token, start + 1) == prefix.marker) {
                return integerFromDigits(token, start + 2, prefix.radix, QString::fromLatin1(prefix.name), negative);
            }
        }
    }
    return parseDecimal(token, start, negative);
}

}
