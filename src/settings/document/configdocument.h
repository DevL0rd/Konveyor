#pragma once

#include "document/nodepath.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <expected>
#include <string>
#include <utility>

namespace Konveyor::Settings
{

using EditResult = std::expected<QString, QString>;

class ConfigDocument
{
public:
    explicit ConfigDocument(const QString &text = QString());

    QString text() const;

    const Kdl::Node *find(const QString &path) const;
    QVariantList childPaths(const QString &parentPath, const QString &name) const;

    EditResult setNode(const QString &path, const QVariantMap &node);
    EditResult remove(const QString &path);
    EditResult append(const QString &parentPath, const QVariantMap &node);
    EditResult move(const QString &path, int delta);

private:
    struct Located
    {
        NodePath path;
        const Kdl::Node *node = nullptr;
    };

    std::expected<Located, QString> locate(const QString &path) const;
    EditResult ensure(const NodePath &path);
    EditResult insertChild(const NodePath &parentPath, const QVariantMap &node);
    static bool sameChildNames(const QList<Kdl::Node> &children, const QVariantMap &node);
    EditResult updateInPlace(const NodePath &path, const QVariantMap &node);
    EditResult commit(std::u32string text, const QString &resultPath);
    QString slice(qsizetype from, qsizetype to) const;
    qsizetype size() const;
    char32_t at(qsizetype index) const;
    qsizetype entryEnd(qsizetype position) const;
    qsizetype attachedCommentStart(qsizetype start, qsizetype lineEnd) const;
    qsizetype commentBlockStart(qsizetype start) const;
    qsizetype nextLineStart(qsizetype position) const;
    qsizetype previousLineStart(qsizetype start) const;
    std::pair<qsizetype, qsizetype> ownedRange(const Kdl::Span &span) const;
    QString indentAt(qsizetype position) const;
    QString indentFor(const NodePath &path) const;
    bool usesCrlf() const;
    qsizetype lineStart(qsizetype position) const;
    void replace(std::u32string &text, qsizetype from, qsizetype to, const QString &replacement) const;
    void reparse();

    std::u32string m_text;
    Kdl::Document m_document;
    QString m_parseError;
};

}
