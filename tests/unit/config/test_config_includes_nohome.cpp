#include "config/loader.h"

#include <QDir>
#include <QTest>

using namespace Konveyor::Config;

QString QDir::homePath()
{
    return {};
}

class TestConfigIncludesNoHome : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void theHomeDirectoryIsReplacedByNothing() { QVERIFY(QDir::homePath().isEmpty()); }

    void aTildeIncludeNeedsAHomeDirectory()
    {
        const auto loaded = loadString(QStringLiteral("include \"~/extra.kdl\"\n"), QStringLiteral("/tmp/konveyor-nohome/config.kdl"));
        QVERIFY(!loaded.has_value());
        QVERIFY2(loaded.error().message.contains(QStringLiteral("error retrieving home directory to expand ~/extra.kdl")),
            qPrintable(loaded.error().message));
        QCOMPARE(loaded.error().location.line, 1);
    }

    void aLoneTildeNeedsOneToo()
    {
        const auto loaded = loadString(QStringLiteral("include \"~\"\n"), QStringLiteral("/tmp/konveyor-nohome/config.kdl"));
        QVERIFY(!loaded.has_value());
        QVERIFY2(loaded.error().message.contains(QStringLiteral("error retrieving home directory to expand ~")),
            qPrintable(loaded.error().message));
    }
};

QTEST_GUILESS_MAIN(TestConfigIncludesNoHome)
#include "test_config_includes_nohome.moc"
