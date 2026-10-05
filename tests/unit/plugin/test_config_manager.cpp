#include "plugin/configmanager.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using Konveyor::ConfigManager;

namespace
{

QString gapsConfig(int gaps)
{
    return QStringLiteral("layout {\n    gaps %1\n}\n").arg(gaps);
}

bool writeInPlace(const QString &path, const QString &text)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) && file.write(text.toUtf8()) >= 0;
}

bool replaceByRename(const QString &path, const QString &text)
{
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Text) && file.write(text.toUtf8()) >= 0 && file.commit();
}

}

class TestConfigManager : public QObject
{
    Q_OBJECT

public:
    static void initMain() { qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/konveyor-test-bus"); }

private Q_SLOTS:
    void init();
    void reloadsAConfigEditedInPlace();
    void reloadsForAnEditNotYetReportedWhenALoadFinishes();
    void keepsWatchingAConfigReplacedByRename();
    void watchesAnIncludeAddedLater();
    void reloadsWhenABrokenIncludeIsFixed_data();
    void followsASymlinkedConfigAcrossReplacements();
    void reloadsWhenABrokenIncludeIsFixed();
    void keepsFollowingAConfigLoadedFromAnotherPath();

private:
    QString filePath(const QString &name) const;

    std::unique_ptr<QTemporaryDir> m_dir;
};

QString TestConfigManager::filePath(const QString &name) const
{
    return m_dir->filePath(name);
}

void TestConfigManager::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    qputenv("KONVEYOR_CONFIG", filePath(QStringLiteral("config.kdl")).toUtf8());
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(1)));
}

void TestConfigManager::reloadsAConfigEditedInPlace()
{
    ConfigManager manager;
    manager.start();
    QCOMPARE(manager.config().layout.gaps, 1.0);
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(2)));
    QTRY_COMPARE(manager.config().layout.gaps, 2.0);
}

void TestConfigManager::reloadsForAnEditNotYetReportedWhenALoadFinishes()
{
    ConfigManager manager;
    manager.start();
    QSignalSpy changed(&manager, &ConfigManager::configChanged);
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(2)));
    QCOMPARE(manager.load(), QString());
    QCOMPARE(changed.count(), 1);
    QVERIFY(changed.wait());
    QCOMPARE(manager.config().layout.gaps, 2.0);
}

void TestConfigManager::keepsWatchingAConfigReplacedByRename()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(replaceByRename(filePath(QStringLiteral("config.kdl")), gapsConfig(3)));
    QTRY_COMPARE(manager.config().layout.gaps, 3.0);
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(4)));
    QTRY_COMPARE(manager.config().layout.gaps, 4.0);
}

void TestConfigManager::watchesAnIncludeAddedLater()
{
    ConfigManager manager;
    manager.start();
    QVERIFY(writeInPlace(filePath(QStringLiteral("extra.kdl")), gapsConfig(5)));
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(1) + QStringLiteral("include \"extra.kdl\"\n")));
    QTRY_COMPARE(manager.config().layout.gaps, 5.0);
    QVERIFY(writeInPlace(filePath(QStringLiteral("extra.kdl")), gapsConfig(6)));
    QTRY_COMPARE(manager.config().layout.gaps, 6.0);
}

void TestConfigManager::reloadsWhenABrokenIncludeIsFixed_data()
{
    QTest::addColumn<QString>("include");
    QTest::newRow("same folder") << QStringLiteral("extra.kdl");
    QTest::newRow("sub folder") << QStringLiteral("parts/extra.kdl");
}

void TestConfigManager::reloadsWhenABrokenIncludeIsFixed()
{
    QFETCH(QString, include);
    QVERIFY(QDir(m_dir->path()).mkpath(QStringLiteral("parts")));
    QVERIFY(writeInPlace(filePath(include), gapsConfig(5)));
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(1) + QStringLiteral("include \"%1\"\n").arg(include)));
    ConfigManager manager;
    manager.start();
    QCOMPARE(manager.config().layout.gaps, 5.0);
    QSignalSpy loaded(&manager, &ConfigManager::configLoaded);
    QVERIFY(writeInPlace(filePath(include), QStringLiteral("layout {\n")));
    QVERIFY(loaded.wait());
    QCOMPARE(loaded.last().first().toBool(), true);
    QVERIFY(writeInPlace(filePath(include), gapsConfig(7)));
    QTRY_COMPARE(manager.config().layout.gaps, 7.0);
}

void TestConfigManager::followsASymlinkedConfigAcrossReplacements()
{
    QVERIFY(QDir(m_dir->path()).mkpath(QStringLiteral("dotfiles")));
    const QString target = filePath(QStringLiteral("dotfiles/config.kdl"));
    QVERIFY(writeInPlace(target, gapsConfig(2)));
    QVERIFY(QFile::remove(filePath(QStringLiteral("config.kdl"))));
    QVERIFY(QFile::link(target, filePath(QStringLiteral("config.kdl"))));
    ConfigManager manager;
    manager.start();
    QCOMPARE(manager.config().layout.gaps, 2.0);
    QVERIFY(replaceByRename(target, gapsConfig(3)));
    QTRY_COMPARE(manager.config().layout.gaps, 3.0);
    QVERIFY(writeInPlace(target, gapsConfig(4)));
    QTRY_COMPARE(manager.config().layout.gaps, 4.0);
}

void TestConfigManager::keepsFollowingAConfigLoadedFromAnotherPath()
{
    ConfigManager manager;
    manager.start();
    const QString other = filePath(QStringLiteral("other.kdl"));
    QVERIFY(writeInPlace(other, gapsConfig(5)));
    QCOMPARE(manager.load(other), QString());
    QCOMPARE(manager.config().layout.gaps, 5.0);
    QList<double> applied;
    connect(&manager, &ConfigManager::configChanged, this,
        [&applied](const Konveyor::Config::Config &config) { applied.append(config.layout.gaps); });
    QVERIFY(writeInPlace(filePath(QStringLiteral("config.kdl")), gapsConfig(7)));
    QVERIFY(writeInPlace(other, gapsConfig(6)));
    QTRY_COMPARE(manager.config().layout.gaps, 6.0);
    QVERIFY2(!applied.contains(7.0), "an edit of the main config switched back to it");
}

QTEST_GUILESS_MAIN(TestConfigManager)
#include "test_config_manager.moc"
