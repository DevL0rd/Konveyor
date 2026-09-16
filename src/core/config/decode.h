#pragma once

#include "kdl/kdl.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

#include <functional>

namespace Konveyor::Config
{

struct DecodeError
{
    QString message;
    Kdl::Location location;
};

struct Range
{
    qint64 min = 0;
    qint64 max = 0;
};

inline constexpr Range AnyNumber {-2147483648LL, 2147483647LL};

using NodeHandler = std::function<void(const Kdl::Node &)>;
using ValueHandler = std::function<void(const Kdl::Value &)>;
using NodeTable = QHash<QString, NodeHandler>;
using ValueTable = QHash<QString, ValueHandler>;

[[noreturn]] void failAt(const Kdl::Location &location, const QString &message);
[[noreturn]] void fail(const Kdl::Node &node, const QString &message);

QString quoteName(const QString &name);

void expectNoArguments(const Kdl::Node &node);
void expectNoProperties(const Kdl::Node &node);
void expectNoChildren(const Kdl::Node &node);
void expectOnlyChildren(const Kdl::Node &node);
void expectLeafNode(const Kdl::Node &node);
void expectArgumentLimit(const Kdl::Node &node, int limit);

const Kdl::Value &requiredArgument(const Kdl::Node &node, const QString &name);

double toNumber(const Kdl::Value &value, Range range);
qint64 toInteger(const Kdl::Value &value, Range range);
QString toText(const Kdl::Value &value);
bool toBoolean(const Kdl::Value &value);
QString toWritten(const Kdl::Value &value);
int toKeyword(const Kdl::Value &value, const QStringList &keywords);

double numberArgument(const Kdl::Node &node, Range range);
qint64 integerArgument(const Kdl::Node &node, Range range);
QString stringArgument(const Kdl::Node &node);
bool booleanArgument(const Kdl::Node &node);
bool flagArgument(const Kdl::Node &node);
int keywordArgument(const Kdl::Node &node, const QStringList &keywords);

void decodeChildren(const Kdl::Node &node, const NodeTable &table, const QSet<QString> &repeatable = {});
void decodeProperties(const Kdl::Node &node, const ValueTable &table);

}
