#pragma once

#include "kdl/kdl.h"

#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <string>

namespace Konveyor::Settings
{

std::u32string toCodePoints(const QString &text);
QVariantMap nodeToVariant(const Kdl::Node &node);
QVariantMap withPropertyOrder(QVariantMap node, const Kdl::Node &existing);
QString writeNodeHead(const QVariantMap &node);
QString writeNode(const QVariantMap &node, const QString &indent);
QString writeValue(const QVariant &value);
QString writeIdentifier(const QString &name);

}
