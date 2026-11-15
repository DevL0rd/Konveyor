#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigOverview : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaults();
    void parsesValues();
    void bareFlag();
    void offKeepsDefaults();
    void rejectsOutOfRangeValues_data();
    void rejectsOutOfRangeValues();
    void rejectsUnknownChildren();
    void rejectsArguments();
};

void TestConfigOverview::defaults()
{
    const Overview overview = defaultConfig().overview;
    QCOMPARE(overview.enabled, false);
    QCOMPARE(overview.minimumScale, 0.3);
    QCOMPARE(overview.cardGap, 0.08);
    QCOMPARE(overview.offscreenIndicators, OverviewIndicators::Chevrons);
}

void TestConfigOverview::parsesValues()
{
    const Overview overview = parsed(QStringLiteral(R"(
        overview {
            enabled true
            minimum-scale 0.45
            card-gap 0.12
            offscreen-indicators "off"
        }
    )"))
                                  .overview;
    QCOMPARE(overview.enabled, true);
    QCOMPARE(overview.minimumScale, 0.45);
    QCOMPARE(overview.cardGap, 0.12);
    QCOMPARE(overview.offscreenIndicators, OverviewIndicators::Off);
}

void TestConfigOverview::bareFlag()
{
    QVERIFY(parsed(QStringLiteral("overview { enabled; }")).overview.enabled);
}

void TestConfigOverview::offKeepsDefaults()
{
    QCOMPARE(parsed(QStringLiteral("overview { enabled false; }")).overview, defaultConfig().overview);
}

void TestConfigOverview::rejectsOutOfRangeValues_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("message");
    QTest::newRow("scale too small") << QStringLiteral("overview { minimum-scale 0.05; }")
                                     << QStringLiteral("minimum-scale must be between 0.1 and 1");
    QTest::newRow("scale too large") << QStringLiteral("overview { minimum-scale 1.5; }")
                                     << QStringLiteral("value must be between 0 and 1");
    QTest::newRow("gap too large") << QStringLiteral("overview { card-gap 0.75; }") << QStringLiteral("card-gap must be between 0 and 0.5");
    QTest::newRow("negative gap") << QStringLiteral("overview { card-gap -0.1; }") << QStringLiteral("value must be between 0 and 1");
}

void TestConfigOverview::rejectsOutOfRangeValues()
{
    QFETCH(QString, text);
    QFETCH(QString, message);
    QVERIFY(mustFail(text).message.contains(message));
}

void TestConfigOverview::rejectsUnknownChildren()
{
    verifyFailure(QStringLiteral("overview { »scale 0.5; }"), QStringLiteral("unexpected node `scale`"));
}

void TestConfigOverview::rejectsArguments()
{
    verifyFailure(QStringLiteral("overview »true { }"), QStringLiteral("no arguments expected for this node"));
}

QTEST_MAIN(TestConfigOverview)
#include "test_config_overview.moc"
