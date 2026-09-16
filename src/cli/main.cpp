#include "format.h"
#include "validate.h"

#include "config/loader.h"
#include "ipc/dbusnames.h"
#include "ipc/model.h"
#include "shortcutconflicts.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonDocument>
#include <QTextStream>

#include <functional>
#include <map>

namespace
{

using Formatter = QString (*)(const QJsonDocument &);

struct QueryCommand
{
    QString method;
    Formatter formatter;
    QString help;
};

const std::map<QString, QueryCommand> &queryCommands()
{
    static const std::map<QString, QueryCommand> commands {
        {QStringLiteral("windows"), {QStringLiteral("Windows"), Konveyor::Cli::formatWindows, QStringLiteral("List open windows")}},
        {QStringLiteral("workspaces"), {QStringLiteral("Workspaces"), Konveyor::Cli::formatWorkspaces, QStringLiteral("List workspaces")}},
        {QStringLiteral("outputs"), {QStringLiteral("Outputs"), Konveyor::Cli::formatOutputs, QStringLiteral("List outputs")}},
        {QStringLiteral("focused-window"),
            {QStringLiteral("FocusedWindow"), Konveyor::Cli::formatFocusedWindow, QStringLiteral("Print the focused window")}},
        {QStringLiteral("focused-output"),
            {QStringLiteral("FocusedOutput"), Konveyor::Cli::formatOutputs, QStringLiteral("Print the focused output")}},
        {QStringLiteral("binds"), {QStringLiteral("Binds"), Konveyor::Cli::formatBinds, QStringLiteral("List configured key binds")}},
    };
    return commands;
}

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

int printUsage()
{
    out() << "Usage: konveyor <command>\n\n"
          << "Commands:\n"
          << "  msg [--json] <request>   Communicate with the running Konveyor instance\n"
          << "  validate [-c <path>]     Validate the config file\n"
          << "  restore-shortcuts        Give back the KDE shortcuts Konveyor took over\n\n"
          << "Requests:\n";
    for (const auto &[name, command] : queryCommands()) {
        out() << "  " << name.leftJustified(24) << command.help << "\n";
    }
    out() << "  " << QStringLiteral("action <name> [args]").leftJustified(24) << "Perform an action, e.g. `action set-column-width +10%`\n"
          << "  " << QStringLiteral("load-config-file").leftJustified(24) << "Reload the config file\n"
          << "  " << QStringLiteral("version").leftJustified(24) << "Print the running version\n";
    out().flush();
    return 0;
}

QDBusInterface &interface()
{
    static QDBusInterface instance(
        Konveyor::Ipc::dbusService, Konveyor::Ipc::dbusPath, Konveyor::Ipc::dbusInterface, QDBusConnection::sessionBus());
    return instance;
}

int fail(const QString &message)
{
    err() << "Error: " << message << "\n";
    err().flush();
    return 1;
}

std::optional<QString> callString(const QString &method, const QVariantList &arguments = {})
{
    const QDBusReply<QString> reply = interface().callWithArgumentList(QDBus::Block, method, arguments);
    if (!reply.isValid()) {
        fail(QStringLiteral("could not reach Konveyor (is the effect enabled?): %1").arg(reply.error().message()));
        return std::nullopt;
    }
    return reply.value();
}

int runQuery(const QueryCommand &command, bool json)
{
    const std::optional<QString> reply = callString(command.method);
    if (!reply) {
        return 1;
    }
    const QJsonDocument document = QJsonDocument::fromJson(reply->toUtf8());
    out() << (json ? QString::fromUtf8(document.toJson(QJsonDocument::Compact)) : command.formatter(document)) << "\n";
    out().flush();
    return 0;
}

int runErrorReturningCall(const QString &method, const QVariantList &arguments)
{
    const std::optional<QString> reply = callString(method, arguments);
    if (!reply) {
        return 1;
    }
    return reply->isEmpty() ? 0 : fail(*reply);
}

int runAction(const QStringList &arguments)
{
    const auto request = Konveyor::Ipc::actionFromArguments(arguments);
    if (!request) {
        return fail(request.error());
    }
    const QString json = QString::fromUtf8(QJsonDocument(Konveyor::Ipc::actionToJson(*request)).toJson(QJsonDocument::Compact));
    return runErrorReturningCall(QStringLiteral("Action"), {json});
}

QStringList subcommandArguments(const QString &command, const QStringList &arguments)
{
    return QStringList {QStringLiteral("konveyor %1").arg(command)} + arguments;
}

int runMsg(const QStringList &arguments)
{
    QCommandLineParser parser;
    const QCommandLineOption jsonOption(QStringLiteral("json"), QStringLiteral("Print raw JSON"));
    parser.addOption(jsonOption);
    parser.addPositionalArgument(QStringLiteral("request"), QStringLiteral("The request to send"));
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    if (!parser.parse(subcommandArguments(QStringLiteral("msg"), arguments))) {
        return fail(parser.errorText());
    }
    QStringList positional = parser.positionalArguments();
    if (positional.isEmpty()) {
        return printUsage();
    }
    const bool json = parser.isSet(jsonOption);
    const QString request = positional.takeFirst();
    if (const auto it = queryCommands().find(request); it != queryCommands().end()) {
        return runQuery(it->second, json);
    }
    const std::map<QString, std::function<int()>> handlers {
        {QStringLiteral("action"), [&] { return runAction(positional); }},
        {QStringLiteral("load-config-file"),
            [&] { return runErrorReturningCall(QStringLiteral("LoadConfigFile"), {positional.value(0)}); }},
        {QStringLiteral("version"),
            [&] {
                return runQuery({QStringLiteral("Version"),
                                    [](const QJsonDocument &d) { return d.object().value(QStringLiteral("version")).toString(); }, {}},
                    json);
            }},
    };
    if (const auto it = handlers.find(request); it != handlers.end()) {
        return it->second();
    }
    return fail(QStringLiteral("unknown request: %1").arg(request));
}

int runValidateCommand(const QStringList &arguments)
{
    QCommandLineParser parser;
    const QCommandLineOption configOption(
        {QStringLiteral("c"), QStringLiteral("config")}, QStringLiteral("Config file to validate"), QStringLiteral("path"));
    parser.addOption(configOption);
    if (!parser.parse(subcommandArguments(QStringLiteral("validate"), arguments))) {
        return fail(parser.errorText());
    }
    const QString path = parser.isSet(configOption) ? parser.value(configOption) : Konveyor::Config::configPath();
    return Konveyor::Cli::runValidate(path, out(), err());
}

int runRestoreShortcuts()
{
    const QList<Konveyor::ReleasedShortcut> released = Konveyor::ShortcutConflicts::load();
    Konveyor::ShortcutConflicts::restore(released);
    Konveyor::ShortcutConflicts::save({});
    out() << "Restored " << released.size() << " KDE shortcuts\n";
    out().flush();
    return 0;
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("command"), QStringLiteral("msg, validate or restore-shortcuts"));
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    const QCommandLineOption helpOption = parser.addHelpOption();
    if (!parser.parse(app.arguments())) {
        return fail(parser.errorText());
    }
    QStringList positional = parser.positionalArguments();
    if (parser.isSet(helpOption) || positional.isEmpty()) {
        return printUsage();
    }
    const QString command = positional.takeFirst();
    if (command == QLatin1String("msg")) {
        return runMsg(positional);
    }
    if (command == QLatin1String("validate")) {
        return runValidateCommand(positional);
    }
    if (command == QLatin1String("restore-shortcuts")) {
        return runRestoreShortcuts();
    }
    return fail(QStringLiteral("unknown command: %1").arg(command));
}
