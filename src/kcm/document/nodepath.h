#pragma once

#include "kdl/kdl.h"

#include <QList>
#include <QString>

#include <expected>

namespace Konveyor::Settings
{

struct PathSegment
{
    QString name;
    qsizetype index = 0;
};

using NodePath = QList<PathSegment>;

std::expected<NodePath, QString> parsePath(const QString &text);
QString formatPath(const NodePath &path);
const Kdl::Node *findChild(const QList<Kdl::Node> &nodes, const PathSegment &segment);
const Kdl::Node *findNode(const Kdl::Document &document, const NodePath &path);
qsizetype countNamed(const QList<Kdl::Node> &nodes, const QString &name);
const QList<Kdl::Node> &childrenOf(const Kdl::Document &document, const Kdl::Node *parent);

}
