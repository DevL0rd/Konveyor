#pragma once

#include "kdl/kdl.h"

#include <QString>

namespace Konveyor::Kdl::Testing
{

inline constexpr QStringView testFileName = u"test.kdl";

inline std::expected<Document, ParseError> parseText(const QString &text)
{
    return parse(text, testFileName.toString());
}

inline QByteArray describeFailure(const std::expected<Document, ParseError> &result)
{
    return result ? QByteArray("unexpected success") : result.error().toString().toUtf8();
}

}
