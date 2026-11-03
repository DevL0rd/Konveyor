#include "configfiles.h"
#include "filefaults.h"
#include "plugin/configmanager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using Konveyor::ConfigManager;
using Konveyor::Test::gapsConfig;
using Konveyor::Test::isolate;
using Konveyor::Test::Permissions;
using Konveyor::Test::writeInPlace;
using Konveyor::Test::WriteLimit;

namespace
{

constexpr QFileDevice::Permissions unreadable {};
constexpr QFileDevice::Permissions readOnlyDirectory = QFileDevice::ReadOwner | QFileDevice::ExeOwner;

}

class TestConfigManagerEdits : public QObject
{
    Q_OBJECT

public:
    static void initMain() { qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/konveyor-test-bus"); }

private Q_SLOTS:
    void init();
    void writesTheRuleAndTheIncludeOnlyOnce();
    void failsToSetARuleWhenTheOverrideFileIsUnreadable();
    void failsToSetARuleWhenTheOverrideFileIsBroken();
    void failsToSetARuleWhenTheConfigIsGone();
    void failsToSetARuleWhenTheConfigIsBroken();
    void failsToSetARuleWhenTheFolderCannotBeWritten();
    void failsToSetARuleWhenTheWrittenConfigDoesNotLoad();
    void failsToSetARuleWhenTheWriteIsCutShort();
    void rewritesOnlyWhatTheEditChanged();
    void failsToRewriteWhenTheConfigIsGone();
    void failsToRewriteWhenTheEditFails();
    void refusesAnEditThatBreaksTheConfig();
    void readsItsOwnText();
    void skipsMigrationsWhenTheAppliedListIsUnreadable();
    void stopsMigratingWhenTheAppliedListCannotBeSaved();
    void reportsAFailureToCreateTheMissingConfig();
    void createsTheMissingConfigFromTheBundledDefault();
    void reportsAMissingBundledDefault();
    void keepsTheMainConfigWatchedWhenAnotherFileFailsToLoad();
    void staysQuietAboutAFailureWhenTheConfigAsksForIt();
    void loadsAConfigThatOnlyWarns();

private:
    QString filePath(const QString &name) const;
    QString configPath() const { return filePath(QStringLiteral("config.kdl")); }
    void installDefaults() const;
    QString fileText(const QString &name) const;

    std::unique_ptr<QTemporaryDir> m_dir;
};

QString TestConfigManagerEdits::filePath(const QString &name) const
{
    return m_dir->filePath(name);
}

void TestConfigManagerEdits::installDefaults() const
{
    QVERIFY(QDir().mkpath(filePath(QStringLiteral("data/konveyor"))));
    QVERIFY(QFile::copy(
        QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"), filePath(QStringLiteral("data/konveyor/default-config.kdl"))));
}

QString TestConfigManagerEdits::fileText(const QString &name) const
{
    QFile file(filePath(name));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

void TestConfigManagerEdits::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    isolate(m_dir->path());
    qputenv("KONVEYOR_CONFIG", configPath().toUtf8());
    QVERIFY(writeInPlace(configPath(), gapsConfig(1)));
}

void TestConfigManagerEdits::writesTheRuleAndTheIncludeOnlyOnce()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(manager.setForceResizable(QStringLiteral("org.example.app"), true).has_value());
    QVERIFY(manager.setForceResizable(QStringLiteral("org.example.other"), false).has_value());
    QCOMPARE(fileText(QStringLiteral("config.kdl")).count(QStringLiteral("include \"force-resizable.kdl\"")), 1);
    const QString override = fileText(QStringLiteral("force-resizable.kdl"));
    QVERIFY(override.contains(QStringLiteral("org\\.example\\.app")));
    QVERIFY(override.contains(QStringLiteral("org\\.example\\.other")));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheOverrideFileIsUnreadable()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(writeInPlace(filePath(QStringLiteral("force-resizable.kdl")), QString()));
    const Permissions locked(filePath(QStringLiteral("force-resizable.kdl")), unreadable);
    const auto result = manager.setForceResizable(QStringLiteral("org.example.app"), true);
    QVERIFY(!result);
    QVERIFY(result.error().contains(QStringLiteral("Could not read")));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheOverrideFileIsBroken()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(writeInPlace(filePath(QStringLiteral("force-resizable.kdl")), QStringLiteral("window-rule {\n")));
    QVERIFY(!manager.setForceResizable(QStringLiteral("org.example.app"), true));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheConfigIsGone()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(QFile::remove(configPath()));
    const auto result = manager.setForceResizable(QStringLiteral("org.example.app"), true);
    QVERIFY(!result);
    QVERIFY(result.error().contains(QStringLiteral("Could not read")));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheConfigIsBroken()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(writeInPlace(configPath(), QStringLiteral("layout {\n")));
    QVERIFY(!manager.setForceResizable(QStringLiteral("org.example.app"), true));
    QCOMPARE(fileText(QStringLiteral("config.kdl")), QStringLiteral("layout {\n"));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheFolderCannotBeWritten()
{
    ConfigManager manager;
    manager.start();
    const Permissions locked(m_dir->path(), readOnlyDirectory);
    const auto result = manager.setForceResizable(QStringLiteral("org.example.app"), true);
    QVERIFY(!result);
    QVERIFY(result.error().contains(QStringLiteral("Could not write")));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheWrittenConfigDoesNotLoad()
{
    QVERIFY(writeInPlace(configPath(), gapsConfig(1) + QStringLiteral("include \"missing.kdl\"\n")));
    ConfigManager manager;
    manager.start();
    const auto result = manager.setForceResizable(QStringLiteral("org.example.app"), true);
    QVERIFY(!result);
    QVERIFY(result.error().contains(QStringLiteral("missing.kdl")));
    QVERIFY(QFile::exists(filePath(QStringLiteral("force-resizable.kdl"))));
}

void TestConfigManagerEdits::failsToSetARuleWhenTheWriteIsCutShort()
{
    ConfigManager manager;
    manager.start();
    const WriteLimit limit(0);
    const auto result = manager.setForceResizable(QStringLiteral("org.example.app"), true);
    QVERIFY(!result);
    QVERIFY(result.error().contains(QStringLiteral("Could not write")));
}

void TestConfigManagerEdits::rewritesOnlyWhatTheEditChanged()
{
    ConfigManager manager;
    manager.start();
    int calls = 0;
    const auto keep = [&calls](const QString &text) -> std::expected<QString, QString> {
        ++calls;
        return text;
    };
    QVERIFY(manager.rewrite(keep).has_value());
    QCOMPARE(calls, 1);
    const auto widen = [](const QString &) -> std::expected<QString, QString> { return gapsConfig(9); };
    QVERIFY(manager.rewrite(widen).has_value());
    QCOMPARE(fileText(QStringLiteral("config.kdl")), gapsConfig(9));
}

void TestConfigManagerEdits::failsToRewriteWhenTheConfigIsGone()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(QFile::remove(configPath()));
    bool edited = false;
    const auto result = manager.rewrite([&edited](const QString &text) -> std::expected<QString, QString> {
        edited = true;
        return text;
    });
    QVERIFY(!result);
    QVERIFY(!edited);
}

void TestConfigManagerEdits::failsToRewriteWhenTheEditFails()
{
    ConfigManager manager;
    manager.start();
    const auto result
        = manager.rewrite([](const QString &) -> std::expected<QString, QString> { return std::unexpected(QStringLiteral("nope")); });
    QVERIFY(!result);
    QCOMPARE(result.error(), QStringLiteral("nope"));
    QCOMPARE(fileText(QStringLiteral("config.kdl")), gapsConfig(1));
}

void TestConfigManagerEdits::refusesAnEditThatBreaksTheConfig()
{
    ConfigManager manager;
    manager.start();
    const auto result = manager.rewrite([](const QString &) -> std::expected<QString, QString> { return QStringLiteral("layout {\n"); });
    QVERIFY(!result);
    QCOMPARE(fileText(QStringLiteral("config.kdl")), gapsConfig(1));
}

void TestConfigManagerEdits::readsItsOwnText()
{
    ConfigManager manager;
    manager.start();
    QCOMPARE(manager.text().value_or(QString()), gapsConfig(1));
    QVERIFY(QFile::remove(configPath()));
    QVERIFY(!manager.text());
}

void TestConfigManagerEdits::skipsMigrationsWhenTheAppliedListIsUnreadable()
{
    installDefaults();
    const QString old = QStringLiteral("layout {\n    focus-ring {\n        width 4\n    }\n}\n");
    QVERIFY(writeInPlace(configPath(), old));
    QVERIFY(QDir().mkpath(filePath(QStringLiteral("state/konveyor"))));
    const QString applied = filePath(QStringLiteral("state/konveyor/config-migrations"));
    QVERIFY(writeInPlace(applied, QString()));
    const Permissions locked(applied, unreadable);
    ConfigManager manager;
    manager.start();
    QCOMPARE(fileText(QStringLiteral("config.kdl")), old);
}

void TestConfigManagerEdits::stopsMigratingWhenTheAppliedListCannotBeSaved()
{
    installDefaults();
    QVERIFY(QDir().mkpath(filePath(QStringLiteral("state"))));
    const Permissions locked(filePath(QStringLiteral("state")), readOnlyDirectory);
    const QString mine = QStringLiteral("binds {\n    Super+Alt+3 { spawn \"mine\"; }\n}\n");
    QVERIFY(writeInPlace(configPath(), mine));
    ConfigManager manager;
    manager.start();
    QCOMPARE(manager.config().binds.size(), 1);
    QVERIFY(!QFile::exists(filePath(QStringLiteral("state/konveyor/config-migrations"))));
}

void TestConfigManagerEdits::reportsAFailureToCreateTheMissingConfig()
{
    installDefaults();
    QVERIFY(QDir().mkpath(filePath(QStringLiteral("locked"))));
    const QString target = filePath(QStringLiteral("locked/config.kdl"));
    qputenv("KONVEYOR_CONFIG", target.toUtf8());
    const Permissions locked(filePath(QStringLiteral("locked")), readOnlyDirectory);
    ConfigManager manager;
    manager.start();
    QVERIFY(!QFile::exists(target));
}

void TestConfigManagerEdits::createsTheMissingConfigFromTheBundledDefault()
{
    installDefaults();
    QVERIFY(QFile::remove(configPath()));
    ConfigManager manager;
    manager.start();
    QCOMPARE(fileText(QStringLiteral("config.kdl")), fileText(QStringLiteral("data/konveyor/default-config.kdl")));
    QVERIFY(QFile::permissions(configPath()).testFlag(QFileDevice::WriteOwner));
}

void TestConfigManagerEdits::reportsAMissingBundledDefault()
{
    QVERIFY(QFile::remove(configPath()));
    ConfigManager manager;
    manager.start();
    QVERIFY(!QFile::exists(configPath()));
}

void TestConfigManagerEdits::keepsTheMainConfigWatchedWhenAnotherFileFailsToLoad()
{
    ConfigManager manager;
    manager.start();
    const QString other = filePath(QStringLiteral("other.kdl"));
    QVERIFY(writeInPlace(other, QStringLiteral("layout {\n")));
    QVERIFY(!manager.load(other).isEmpty());
    QVERIFY(writeInPlace(configPath(), gapsConfig(8)));
    QTRY_COMPARE(manager.config().layout.gaps, 8.0);
}

void TestConfigManagerEdits::staysQuietAboutAFailureWhenTheConfigAsksForIt()
{
    QVERIFY(writeInPlace(configPath(), QStringLiteral("config-notification {\n    disable-failed true\n}\n")));
    ConfigManager manager;
    manager.start();
    QVERIFY(manager.config().configNotificationDisableFailed);
    QVERIFY(writeInPlace(configPath(), QStringLiteral("layout {\n")));
    QVERIFY(!manager.load().isEmpty());
    QVERIFY(manager.config().configNotificationDisableFailed);
}

void TestConfigManagerEdits::loadsAConfigThatOnlyWarns()
{
    QVERIFY(writeInPlace(configPath(), QStringLiteral("include optional=true \"absent.kdl\"\n") + gapsConfig(3)));
    ConfigManager manager;
    manager.start();
    QCOMPARE(manager.config().layout.gaps, 3.0);
}

QTEST_GUILESS_MAIN(TestConfigManagerEdits)
#include "test_config_manager_edits.moc"
