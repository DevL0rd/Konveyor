#pragma once

#include "document/configdocument.h"

#include <QVariantList>
#include <QVariantMap>

namespace Konveyor::Settings::Testing
{

inline QVariantMap leaf(const QString &name, const QVariantList &arguments = {}, const QVariantMap &properties = {})
{
    return {{QStringLiteral("name"), name}, {QStringLiteral("args"), arguments}, {QStringLiteral("props"), properties}};
}

inline QVariantMap block(const QString &name, const QVariantList &children, const QVariantList &arguments = {})
{
    QVariantMap node = leaf(name, arguments);
    node.insert(QStringLiteral("children"), children);
    return node;
}

template<typename Edit> QString edited(const QString &text, Edit edit)
{
    ConfigDocument document(text);
    const EditResult result = edit(document);
    if (!result) {
        return QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
    }
    return document.text();
}

template<typename Edit> QString failure(const QString &text, Edit edit)
{
    ConfigDocument document(text);
    const EditResult result = edit(document);
    if (result) {
        return QStringLiteral("<edit succeeded: ") + document.text() + QLatin1Char('>');
    }
    if (document.text() != text) {
        return QStringLiteral("<failed edit changed the text>");
    }
    return result.error();
}

}
