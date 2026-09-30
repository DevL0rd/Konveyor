#include "helpers.h"

using namespace LayoutTest;

namespace
{

constexpr double Column = 1080.0 - 2 * 16.0;
constexpr double TwoRows = 1080.0 - 3 * 16.0;

Layout::WindowId addSized(Fixture &fixture, const QString &appId, QSizeF minSize, QSizeF maxSize = QSizeF())
{
    Layout::WindowProperties properties = makeWindow(appId, appId);
    properties.minSize = minSize;
    properties.maxSize = maxSize;
    return fixture.addWith(properties);
}

double widthAfter(const QString &change)
{
    Fixture fixture;
    const auto id = fixture.add();
    if (!fixture.perform(QStringLiteral("set-column-width"), {change}).ok) {
        return -1.0;
    }
    return fixture.frame(id).width();
}

}

class TestLayoutResizeLimits : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void columnWidthStaysWithinItsLimits_data()
    {
        QTest::addColumn<QString>("change");
        QTest::addColumn<double>("width");
        QTest::newRow("full proportion") << QStringLiteral("100%") << 1920.0 - 2 * 16.0;
        QTest::newRow("zero pixels is one pixel") << QStringLiteral("0") << 1.0;
        QTest::newRow("one pixel") << QStringLiteral("1") << 1.0;
        QTest::newRow("shrinking below nothing is one pixel") << QStringLiteral("-5000") << 1.0;
        QTest::newRow("a negative proportion is one pixel") << QStringLiteral("-100%") << 1.0;
        QTest::newRow("wider than the output") << QStringLiteral("5000") << 5000.0;
        QTest::newRow("more than the output") << QStringLiteral("150%") << 2840.0;
        QTest::newRow("the largest width") << QStringLiteral("200000") << 100000.0;
    }

    void columnWidthStaysWithinItsLimits()
    {
        QFETCH(QString, change);
        QFETCH(double, width);
        QCOMPARE(widthAfter(change), width);
    }

    void columnWiderThanTheOutputStartsAtItsLeftEdge()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("5000")}).ok);
        QCOMPARE(fixture.frame(id).x(), 0.0);
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("-4000")}).ok);
        QCOMPARE(fixture.frame(id).width(), 1000.0);
        QCOMPARE(fixture.frame(id).x(), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void minimumWidthBeatsANarrowerColumn()
    {
        Fixture fixture;
        const auto id = addSized(fixture, QStringLiteral("wide"), QSizeF(1200, 0));
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("300")}).ok);
        QCOMPARE(fixture.frame(id).width(), 1200.0);
        const auto small = addSized(fixture, QStringLiteral("small"), QSizeF(2500, 0));
        QCOMPARE(fixture.frame(small).width(), 2500.0);
        VERIFY_INVARIANTS(fixture);
    }

    void maximumWidthCapsTheWindowButNotAMaximizedColumn()
    {
        Fixture fixture;
        const auto id = addSized(fixture, QStringLiteral("capped"), QSizeF(), QSizeF(500, 400));
        QCOMPARE(fixture.frame(id).size(), QSizeF(500, 400));
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("1500")}).ok);
        QCOMPARE(fixture.frame(id).width(), 500.0);
        QVERIFY(fixture.perform(QStringLiteral("maximize-column")).ok);
        QCOMPARE(fixture.frame(id).size(), QSizeF(1920 - 32, Column));
        QVERIFY(fixture.perform(QStringLiteral("maximize-column")).ok);
        QCOMPARE(fixture.frame(id).size(), QSizeF(500, 400));
        VERIFY_INVARIANTS(fixture);
    }

    void stackedColumnTakesTheWidestMinimum()
    {
        Fixture fixture;
        const auto narrow = addSized(fixture, QStringLiteral("narrow"), QSizeF(300, 0));
        const auto wide = addSized(fixture, QStringLiteral("wide"), QSizeF(1100, 0));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(fixture.frame(narrow).width(), 1100.0);
        QCOMPARE(fixture.frame(wide).width(), 1100.0);
        VERIFY_INVARIANTS(fixture);
    }

    void windowHeightStaysWithinTheColumn_data()
    {
        QTest::addColumn<QString>("change");
        QTest::addColumn<double>("height");
        QTest::newRow("taller than the column") << QStringLiteral("5000") << TwoRows - 1.0;
        QTest::newRow("the whole column") << QStringLiteral("100%") << TwoRows - 1.0;
        QTest::newRow("grow past the column") << QStringLiteral("+200%") << TwoRows - 1.0;
        QTest::newRow("zero") << QStringLiteral("0") << 1.0;
        QTest::newRow("shrink below nothing") << QStringLiteral("-5000") << 1.0;
        QTest::newRow("a negative proportion") << QStringLiteral("-100%") << 1.0;
        QTest::newRow("grow by pixels") << QStringLiteral("+100") << TwoRows / 2.0 + 100.0;
        QTest::newRow("shrink by pixels") << QStringLiteral("-100") << TwoRows / 2.0 - 100.0;
    }

    void windowHeightStaysWithinTheColumn()
    {
        QFETCH(QString, change);
        QFETCH(double, height);
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {change}).ok);
        QCOMPARE(fixture.frame(bottom).height(), height);
        QCOMPARE(fixture.frame(top).height(), TwoRows - height);
        VERIFY_INVARIANTS(fixture);
    }

    void windowHeightLeavesTheNeighboursTheirMinimum()
    {
        Fixture fixture;
        const auto top = addSized(fixture, QStringLiteral("top"), QSizeF(0, 700));
        const auto bottom = fixture.add(QStringLiteral("bottom"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(fixture.frame(top).height(), 700.0);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("900")}).ok);
        QCOMPARE(fixture.frame(top).height(), 700.0);
        QCOMPARE(fixture.frame(bottom).height(), TwoRows - 700.0);
        VERIFY_INVARIANTS(fixture);
    }

    void windowHeightKeepsTheWindowsOwnLimits()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("top"));
        const auto limited = addSized(fixture, QStringLiteral("limited"), QSizeF(0, 200), QSizeF(0, 600));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("50")}).ok);
        QCOMPARE(fixture.frame(limited).height(), 200.0);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("900")}).ok);
        QCOMPARE(fixture.frame(limited).height(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }

    void presetHeightTallerThanTheColumnIsCapped()
    {
        Config::Config config = instantConfig();
        config.layout.presetWindowHeights = {Config::PresetSize(Config::Fixed {3000})};
        Fixture fixture(config);
        const auto [top, bottom] = addStackedPair(fixture);
        QVERIFY(fixture.perform(QStringLiteral("switch-preset-window-height")).ok);
        QCOMPARE(fixture.frame(bottom).height(), TwoRows - 1.0);
        QCOMPARE(fixture.frame(top).height(), 1.0);
        QVERIFY(fixture.perform(QStringLiteral("reset-window-height")).ok);
        QCOMPARE(fixture.frame(bottom).height(), TwoRows / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void tallWindowMovedToASmallerMonitorFitsItsColumn()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720));
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
        const auto [top, bottom] = addStackedPair(fixture);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("900")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-monitor"), {QStringLiteral("DP-2")}).ok);
        QCOMPARE(fixture.state(bottom).output, QStringLiteral("DP-2"));
        QCOMPARE(fixture.frame(bottom).height(), 720.0 - 2 * 16.0);
        QCOMPARE(fixture.frame(top).height(), Column);
        VERIFY_INVARIANTS(fixture);
    }

    void heightsOfAThreeRowColumnAddUpToTheColumn()
    {
        Fixture fixture;
        const auto [top, middle] = addStackedPair(fixture);
        const auto bottom = fixture.add(QStringLiteral("bottom"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("50%")}).ok);
        const double sum = fixture.frame(top).height() + fixture.frame(middle).height() + fixture.frame(bottom).height();
        QCOMPARE(sum, 1080.0 - 4 * 16.0);
        QVERIFY(std::abs(fixture.frame(top).height() - fixture.frame(middle).height()) <= 1.0);
        QVERIFY(fixture.frame(bottom).height() > fixture.frame(top).height());
        VERIFY_INVARIANTS(fixture);
    }

    void tabbedColumnsResizeEveryTabTogether()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        QVERIFY(fixture.perform(QStringLiteral("toggle-column-tabbed-display")).ok);
        QVERIFY(fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("400")}).ok);
        QCOMPARE(fixture.frame(bottom).height(), 400.0);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-up")).ok);
        QCOMPARE(fixture.frame(top).height(), 400.0);
        QVERIFY(fixture.perform(QStringLiteral("reset-window-height")).ok);
        QVERIFY(fixture.frame(top).height() > 400.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutResizeLimits)
#include "test_layout_resizelimits.moc"
