#include "config/loader.h"
#include "values/livematching.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;

class TestSettingsProfiles : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void profileMatchSizeBoundsAreStrict_data();
    void profileMatchSizeBoundsAreStrict();
};

void TestSettingsProfiles::profileMatchSizeBoundsAreStrict_data()
{
    QTest::addColumn<QString>("match");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<bool>("matches");
    QTest::newRow("wider than") << QStringLiteral("width-above=2560") << 3440 << 1440 << true;
    QTest::newRow("exactly as wide") << QStringLiteral("width-above=2560") << 2560 << 1440 << false;
    QTest::newRow("narrower than") << QStringLiteral("width-below=1920") << 1280 << 720 << true;
    QTest::newRow("exactly the width limit") << QStringLiteral("width-below=1920") << 1920 << 1080 << false;
    QTest::newRow("taller than") << QStringLiteral("height-above=1440") << 2560 << 1600 << true;
    QTest::newRow("exactly as tall") << QStringLiteral("height-above=1440") << 2560 << 1440 << false;
    QTest::newRow("shorter than") << QStringLiteral("height-below=1080") << 1280 << 800 << true;
    QTest::newRow("too tall") << QStringLiteral("height-below=1080") << 1920 << 1200 << false;
    QTest::newRow("inside a width range") << QStringLiteral("width-above=1000 width-below=2000") << 1920 << 1080 << true;
    QTest::newRow("outside a width range") << QStringLiteral("width-above=1000 width-below=2000") << 2560 << 1440 << false;
    QTest::newRow("one of several match lines") << QStringLiteral("width-above=5000; match height-below=900") << 1280 << 800 << true;
    QTest::newRow("none of several match lines") << QStringLiteral("width-above=5000; match height-below=900") << 1920 << 1080 << false;
}

void TestSettingsProfiles::profileMatchSizeBoundsAreStrict()
{
    QFETCH(QString, match);
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(bool, matches);
    const auto config = Config::loadString(
        QStringLiteral("monitor-profile \"sized\" { match %1; }\nmonitor-profile \"rest\"\n").arg(match), QStringLiteral("config.kdl"));
    QVERIFY(config);
    const QVariantMap output {{QStringLiteral("name"), QStringLiteral("DP-1")},
        {QStringLiteral("logical"), QVariantMap {{QStringLiteral("width"), width}, {QStringLiteral("height"), height}}}};
    QCOMPARE(profileNameFor(config->config, output), matches ? QStringLiteral("sized") : QStringLiteral("rest"));
}

QTEST_GUILESS_MAIN(TestSettingsProfiles)
#include "test_settings_profiles.moc"
