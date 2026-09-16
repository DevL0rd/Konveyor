#include "format.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

namespace Konveyor::Cli
{

namespace
{

QString flags(const QJsonObject &object, const QList<std::pair<QString, QString>> &names)
{
    QStringList active;
    for (const auto &[key, label] : names) {
        if (object.value(key).toBool()) {
            active.append(label);
        }
    }
    return active.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(active.join(QStringLiteral(", ")));
}

QString formatWindow(const QJsonObject &window)
{
    const QJsonObject layout = window.value(QStringLiteral("layout")).toObject();
    const QJsonArray position = layout.value(QStringLiteral("pos_in_scrolling_layout")).toArray();
    const QJsonArray size = layout.value(QStringLiteral("tile_size")).toArray();
    QStringList lines;
    lines << QStringLiteral("Window ID %1:%2")
                 .arg(window.value(QStringLiteral("id")).toInteger())
                 .arg(flags(window,
                     {{QStringLiteral("is_focused"), QStringLiteral("focused")},
                         {QStringLiteral("is_floating"), QStringLiteral("floating")},
                         {QStringLiteral("is_urgent"), QStringLiteral("urgent")}}));
    lines << QStringLiteral("  Title: \"%1\"").arg(window.value(QStringLiteral("title")).toString());
    lines << QStringLiteral("  App ID: \"%1\"").arg(window.value(QStringLiteral("app_id")).toString());
    lines << QStringLiteral("  PID: %1").arg(window.value(QStringLiteral("pid")).toInteger());
    lines << QStringLiteral("  Workspace ID: %1").arg(window.value(QStringLiteral("workspace_id")).toInteger());
    if (!position.isEmpty()) {
        lines << QStringLiteral("  Scrolling position: column %1, tile %2").arg(position.at(0).toInteger()).arg(position.at(1).toInteger());
    }
    if (size.size() == 2) {
        lines << QStringLiteral("  Tile size: %1 x %2").arg(size.at(0).toDouble()).arg(size.at(1).toDouble());
    }
    return lines.join(QLatin1Char('\n'));
}

QString formatWorkspace(const QJsonObject &workspace)
{
    const QString marker = workspace.value(QStringLiteral("is_focused")).toBool()
        ? QStringLiteral(" * ")
        : (workspace.value(QStringLiteral("is_active")).toBool() ? QStringLiteral(" - ") : QStringLiteral("   "));
    const QJsonValue name = workspace.value(QStringLiteral("name"));
    const QString label = name.isString() ? QStringLiteral(" \"%1\"").arg(name.toString()) : QString();
    return QStringLiteral("%1%2%3").arg(marker).arg(workspace.value(QStringLiteral("idx")).toInt()).arg(label);
}

QString joinArray(const QJsonDocument &document, QString (*formatter)(const QJsonObject &), const QString &separator)
{
    QStringList parts;
    const QJsonArray array = document.array();
    for (const auto &item : array) {
        parts.append(formatter(item.toObject()));
    }
    return parts.join(separator);
}

QString formatOutput(const QJsonObject &output)
{
    const QJsonObject logical = output.value(QStringLiteral("logical")).toObject();
    return QStringLiteral("Output \"%1\" (%2)\n  Logical position: %3, %4\n  Logical size: %5x%6\n  Scale: %7")
        .arg(output.value(QStringLiteral("name")).toString(), output.value(QStringLiteral("description")).toString())
        .arg(logical.value(QStringLiteral("x")).toDouble())
        .arg(logical.value(QStringLiteral("y")).toDouble())
        .arg(logical.value(QStringLiteral("width")).toDouble())
        .arg(logical.value(QStringLiteral("height")).toDouble())
        .arg(logical.value(QStringLiteral("scale")).toDouble());
}

QString formatBind(const QJsonObject &bind)
{
    const QJsonObject action = bind.value(QStringLiteral("action")).toObject();
    QStringList parts {action.value(QStringLiteral("name")).toString()};
    const QJsonArray arguments = action.value(QStringLiteral("arguments")).toArray();
    for (const auto &argument : arguments) {
        parts.append(argument.toString());
    }
    const QJsonValue title = bind.value(QStringLiteral("title"));
    const QString description = title.isString() ? title.toString() : parts.join(QLatin1Char(' '));
    return QStringLiteral("%1  %2").arg(bind.value(QStringLiteral("key")).toString(), -32).arg(description);
}

}

QString formatWindows(const QJsonDocument &document)
{
    return joinArray(document, formatWindow, QStringLiteral("\n\n"));
}

QString formatWorkspaces(const QJsonDocument &document)
{
    QStringList lines;
    QString currentOutput;
    const QJsonArray array = document.array();
    for (const auto &item : array) {
        const QJsonObject workspace = item.toObject();
        const QString output = workspace.value(QStringLiteral("output")).toString();
        if (output != currentOutput) {
            lines << QStringLiteral("Output \"%1\":").arg(output);
            currentOutput = output;
        }
        lines << formatWorkspace(workspace);
    }
    return lines.join(QLatin1Char('\n'));
}

QString formatOutputs(const QJsonDocument &document)
{
    return joinArray(document, formatOutput, QStringLiteral("\n\n"));
}

QString formatFocusedWindow(const QJsonDocument &document)
{
    return document.isObject() ? formatWindow(document.object()) : QStringLiteral("No window is focused.");
}

QString formatBinds(const QJsonDocument &document)
{
    return joinArray(document, formatBind, QStringLiteral("\n"));
}

}
