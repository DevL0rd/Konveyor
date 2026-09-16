#pragma once

#include "config/types.h"
#include "layout/common/layouttypes.h"

#include <QString>

#include <expected>

namespace Konveyor::Layout
{

std::expected<SizeChange, QString> parseSizeChange(const QString &text);
std::expected<PositionChange, QString> parsePositionChange(const QString &text);
std::expected<Config::WorkspaceReference, QString> parseWorkspaceReference(const QString &text);
std::expected<Config::ColumnDisplay, QString> parseColumnDisplay(const QString &text);
std::expected<bool, QString> parseBool(const QString &text);
std::expected<quint64, QString> parseIndex(const QString &text);

}
