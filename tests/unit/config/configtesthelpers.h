#pragma once

#include "config/loader.h"

#include <QString>

namespace Konveyor::Config::Testing
{

inline LoadResult mustLoad(const QString &text)
{
    const auto result = loadString(text, QStringLiteral("config.kdl"));
    if (!result) {
        qWarning("unexpected config error: %s", qPrintable(result.error().toString()));
        return LoadResult {};
    }
    return *result;
}

inline Config parsed(const QString &text)
{
    return mustLoad(text).config;
}

inline LoadError mustFail(const QString &text)
{
    const auto result = loadString(text, QStringLiteral("config.kdl"));
    if (result) {
        return LoadError {QStringLiteral("<config loaded without an error>"), Kdl::Location {}, QString(), QStringList {}};
    }
    return result.error();
}

inline double proportionOf(const PresetSize &size)
{
    return std::get<Proportion>(size).value;
}

inline double fixedOf(const PresetSize &size)
{
    return std::get<Fixed>(size).value;
}

}
