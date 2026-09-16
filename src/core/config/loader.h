#pragma once

#include "config/types.h"
#include "kdl/kdl.h"

#include <QString>
#include <QStringList>

#include <expected>

namespace Konveyor::Config
{

struct LoadError
{
    QString message;
    Kdl::Location location;
    QString sourceLine;
    QStringList files;

    QString toString() const;
};

struct LoadResult
{
    Config config;
    QStringList files;
    QStringList warnings;
};

std::expected<LoadResult, LoadError> loadString(const QString &text, const QString &fileName);
std::expected<LoadResult, LoadError> loadFile(const QString &path);
QString configPath();
Config defaultConfig();
Layout mergedLayout(Layout base, const LayoutPart &part);
quint32 keysymFromName(const QString &name);
QString bindKeyLabel(const Bind &bind);

}
