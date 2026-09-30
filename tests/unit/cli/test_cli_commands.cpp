#include "../dbus/fakekglobalaccel.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

#include <memory>

using Konveyor::Test::FakeKGlobalAccel;
using Konveyor::Test::PrivateSession;
using Konveyor::Test::ProgramResult;

namespace
{

bool write(const QString &path, const QByteArray &contents)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

}

class TestCliCommands : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
    }

    void cleanup() { m_session.reset(); }

    void usageListsEveryCommandAndRequest()
    {
        for (const QStringList &arguments : {QStringList {}, QStringList {QStringLiteral("--help")}}) {
            const ProgramResult result = konveyor(arguments);
            QCOMPARE(result.exitCode, 0);
            for (const QString &word : {QStringLiteral("msg [--json] <request>"), QStringLiteral("validate [-c <path>]"),
                     QStringLiteral("restore-shortcuts"), QStringLiteral("windows"), QStringLiteral("workspaces"),
                     QStringLiteral("outputs"), QStringLiteral("focused-window"), QStringLiteral("focused-output"), QStringLiteral("binds"),
                     QStringLiteral("action <name> [args]"), QStringLiteral("load-config-file"), QStringLiteral("version")}) {
                QVERIFY2(result.out.contains(word), qPrintable(word));
            }
        }
    }

    void unknownCommandFails()
    {
        const ProgramResult result = konveyor({QStringLiteral("fly")});
        QCOMPARE(result.exitCode, 1);
        QCOMPARE(result.err, QStringLiteral("Error: unknown command: fly\n"));
        QCOMPARE(konveyor({QStringLiteral("--bogus")}).exitCode, 1);
    }

    void validateAcceptsAGoodConfig()
    {
        const QString path = m_session->dir(QStringLiteral("home")) + QStringLiteral("/good.kdl");
        QVERIFY(write(path, "layout {\n    gaps 8\n}\n"));
        for (const QString &flag : {QStringLiteral("-c"), QStringLiteral("--config")}) {
            const ProgramResult result = konveyor({QStringLiteral("validate"), flag, path});
            QCOMPARE(result.exitCode, 0);
            QCOMPARE(result.out, QStringLiteral("Config is valid: %1\n").arg(path));
            QCOMPARE(result.err, QString());
        }
    }

    void validateAcceptsTheBundledDefault()
    {
        const ProgramResult result
            = konveyor({QStringLiteral("validate"), QStringLiteral("-c"), QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl")});
        QCOMPARE(result.err, QString());
        QCOMPARE(result.exitCode, 0);
    }

    void validateRejectsABadConfig()
    {
        const QString path = m_session->dir(QStringLiteral("home")) + QStringLiteral("/bad.kdl");
        QVERIFY(write(path, "layout {\n    gaps \"wide\"\n}\n"));
        const ProgramResult result = konveyor({QStringLiteral("validate"), QStringLiteral("-c"), path});
        QCOMPARE(result.exitCode, 1);
        QCOMPARE(result.out, QString());
        QVERIFY(!result.err.isEmpty());
    }

    void validateRejectsAMissingFile()
    {
        const ProgramResult result
            = konveyor({QStringLiteral("validate"), QStringLiteral("-c"), QStringLiteral("/nonexistent/config.kdl")});
        QCOMPARE(result.exitCode, 1);
        QVERIFY(result.err.contains(QStringLiteral("/nonexistent/config.kdl")));
    }

    void validatePrintsWarningsButPasses()
    {
        const QString path = m_session->dir(QStringLiteral("home")) + QStringLiteral("/warn.kdl");
        QVERIFY(write(path, "include \"missing.kdl\" optional=true\nlayout {\n    gaps 8\n}\n"));
        const ProgramResult result = konveyor({QStringLiteral("validate"), QStringLiteral("-c"), path});
        QCOMPARE(result.exitCode, 0);
        QCOMPARE(result.err,
            QStringLiteral("warning: optional include not found: %1/missing.kdl\n").arg(m_session->dir(QStringLiteral("home"))));
        QCOMPARE(result.out, QStringLiteral("Config is valid: %1\n").arg(path));
    }

    void validateDefaultsToTheUserConfig()
    {
        const QString path = m_session->dir(QStringLiteral("config")) + QStringLiteral("/konveyor/config.kdl");
        QVERIFY(write(path, "layout {\n    gaps 4\n}\n"));
        const ProgramResult result = konveyor({QStringLiteral("validate")});
        QCOMPARE(result.exitCode, 0);
        QCOMPARE(result.out, QStringLiteral("Config is valid: %1\n").arg(path));

        QProcessEnvironment environment = m_session->environment();
        const QString explicitPath = m_session->dir(QStringLiteral("home")) + QStringLiteral("/explicit.kdl");
        QVERIFY(write(explicitPath, "layout {\n    gaps 2\n}\n"));
        environment.insert(QStringLiteral("KONVEYOR_CONFIG"), explicitPath);
        QCOMPARE(m_session->run(QStringLiteral(KONVEYOR_CLI), {QStringLiteral("validate")}, environment).out,
            QStringLiteral("Config is valid: %1\n").arg(explicitPath));
    }

    void validateNeedsAPathAfterTheFlag()
    {
        const ProgramResult result = konveyor({QStringLiteral("validate"), QStringLiteral("-c")});
        QCOMPARE(result.exitCode, 1);
        QVERIFY(result.err.startsWith(QStringLiteral("Error: ")));
    }

    void restoreShortcutsGivesTheKeysBackAndForgetsThem()
    {
        FakeKGlobalAccel kglobalaccel;
        QVERIFY(kglobalaccel.start(m_session->address()));
        seedReleased();
        const ProgramResult result = konveyor({QStringLiteral("restore-shortcuts")});
        QCOMPARE(result.err, QString());
        QCOMPARE(result.exitCode, 0);
        QCOMPARE(result.out, QStringLiteral("Restored 1 KDE shortcuts\n"));
        QCOMPARE(kglobalaccel.keys(QStringLiteral("kwin"), QStringLiteral("Overview")),
            QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+W"))});
        QVERIFY(releasedGroups().isEmpty());
    }

    void restoreShortcutsWithNothingSavedDoesNothing()
    {
        const ProgramResult result = konveyor({QStringLiteral("restore-shortcuts")});
        QCOMPARE(result.exitCode, 0);
        QCOMPARE(result.out, QStringLiteral("Restored 0 KDE shortcuts\n"));
    }

    void restoreShortcutsKeepsWhatKGlobalAccelRefused()
    {
        seedReleased();
        const ProgramResult result = konveyor({QStringLiteral("restore-shortcuts")});
        QCOMPARE(result.exitCode, 1);
        QCOMPARE(result.out, QStringLiteral("Restored 0 KDE shortcuts\n"));
        QVERIFY(result.err.contains(QStringLiteral("Error: could not restore 1 KDE shortcuts")));
        QCOMPARE(releasedGroups(), QStringList {QStringLiteral("kwin/Overview")});
    }

private:
    ProgramResult konveyor(const QStringList &arguments) const { return m_session->run(QStringLiteral(KONVEYOR_CLI), arguments); }

    QString stateFile() const { return m_session->dir(QStringLiteral("state")) + QStringLiteral("/konveyorstaterc"); }

    void seedReleased() const
    {
        KConfig config(stateFile(), KConfig::SimpleConfig);
        KConfigGroup entry = config.group(QStringLiteral("ReleasedShortcuts")).group(QStringLiteral("kwin/Overview"));
        entry.writeEntry("Component", QStringLiteral("kwin"));
        entry.writeEntry("Action", QStringLiteral("Overview"));
        entry.writeEntry("ComponentFriendlyName", QStringLiteral("KWin"));
        entry.writeEntry("ActionFriendlyName", QStringLiteral("Toggle Overview"));
        entry.writeEntry("Keys", QStringList {QStringLiteral("Meta+W")});
        config.sync();
    }

    QStringList releasedGroups() const
    {
        const KConfig config(stateFile(), KConfig::SimpleConfig);
        return config.group(QStringLiteral("ReleasedShortcuts")).groupList();
    }

    std::unique_ptr<PrivateSession> m_session;
};

QTEST_GUILESS_MAIN(TestCliCommands)
#include "test_cli_commands.moc"
