#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

enum class Kind
{
    Integer,
    Number,
    Positive
};

struct RangeCase
{
    const char *text;
    Kind kind;
    qint64 low;
    qint64 high;
};

constexpr qint64 kIntMax = 2147483647;

constexpr RangeCase kCases[] = {
    {"layout { gaps »%1; }", Kind::Number, 0, 65535},
    {"layout { struts { left »%1; }; }", Kind::Number, -65535, 65535},
    {"layout { struts { right »%1; }; }", Kind::Number, -65535, 65535},
    {"layout { struts { top »%1; }; }", Kind::Number, -65535, 65535},
    {"layout { struts { bottom »%1; }; }", Kind::Number, -65535, 65535},
    {"layout { max-rows-per-column »%1; }", Kind::Integer, 1, 64},
    {"layout { preset-column-widths { proportion »%1; }; }", Kind::Positive, 0, 1},
    {"layout { preset-column-widths { fixed »%1; }; }", Kind::Integer, 1, 65535},
    {"layout { preset-window-heights { proportion »%1; }; }", Kind::Positive, 0, 1},
    {"layout { preset-window-heights { fixed »%1; }; }", Kind::Integer, 1, 65535},
    {"layout { default-column-width { proportion »%1; }; }", Kind::Positive, 0, 1},
    {"layout { default-column-width { fixed »%1; }; }", Kind::Integer, 1, 65535},
    {"layout { focus-ring { width »%1; }; }", Kind::Number, 0, 65535},
    {"layout { border { width »%1; }; }", Kind::Number, 0, 65535},
    {"layout { tab-indicator { gap »%1; }; }", Kind::Number, -65535, 65535},
    {"layout { tab-indicator { width »%1; }; }", Kind::Number, 0, 65535},
    {"layout { tab-indicator { gaps-between-tabs »%1; }; }", Kind::Number, 0, 65535},
    {"layout { tab-indicator { corner-radius »%1; }; }", Kind::Number, 0, 65535},
    {"layout { tab-indicator { length total-proportion=»%1; }; }", Kind::Number, 0, 1},
    {"layout { focus-ring { active-color »%1 0 0 255; }; }", Kind::Integer, 0, 255},
    {"layout { border { inactive-color 0 0 0 »%1; }; }", Kind::Integer, 0, 255},
    {"layout { focus-ring { active-gradient from=\"red\" to=\"blue\" angle=»%1; }; }", Kind::Number, -32768, 32767},
    {"animations { slowdown »%1; }", Kind::Positive, 0, kIntMax},
    {"animations { window-open { duration-ms »%1; }; }", Kind::Integer, 1, kIntMax},
    {"animations { window-resize { spring damping-ratio=1.0 stiffness=»%1 epsilon=0.001; }; }", Kind::Integer, 1, kIntMax},
    {"animations { window-open { curve \"cubic-bezier\" »%1 0 1 1; }; }", Kind::Number, 0, 1},
    {"animations { window-open { curve \"cubic-bezier\" 0 0 »%1 1; }; }", Kind::Number, 0, 1},
    {"gestures { dnd-edge-view-scroll { trigger-width »%1; }; }", Kind::Number, 0, 65535},
    {"gestures { dnd-edge-view-scroll { delay-ms »%1; }; }", Kind::Integer, 0, 65535},
    {"gestures { dnd-edge-view-scroll { max-speed »%1; }; }", Kind::Number, 1, 1000000},
    {"gestures { dnd-edge-workspace-switch { trigger-height »%1; }; }", Kind::Number, 0, 65535},
    {"gestures { dnd-edge-workspace-switch { delay-ms »%1; }; }", Kind::Integer, 0, 65535},
    {"gestures { dnd-edge-workspace-switch { max-speed »%1; }; }", Kind::Number, 1, 1000000},
    {"gestures { touchpad { swipe-fingers »%1; }; }", Kind::Integer, 2, 5},
    {"gestures { touchpad { pinch-fingers »%1; }; }", Kind::Integer, 2, 5},
    {"gestures { touchpad { window-swipe-fingers »%1; }; }", Kind::Integer, 2, 5},
    {"gestures { touchscreen { swipe-fingers »%1; }; }", Kind::Integer, 2, 5},
    {"gestures { touchscreen { pinch-fingers »%1; }; }", Kind::Integer, 2, 5},
    {"gestures { touchscreen { window-swipe-fingers »%1; }; }", Kind::Integer, 2, 5},
    {"gestures { touchscreen { long-press-ms »%1; }; }", Kind::Integer, 100, 5000},
    {"window-rule { max-rows-per-column »%1; }", Kind::Integer, 1, 64},
    {"window-rule { min-width »%1; }", Kind::Integer, 0, 65535},
    {"window-rule { min-height »%1; }", Kind::Integer, 0, 65535},
    {"window-rule { max-width »%1; }", Kind::Integer, 0, 65535},
    {"window-rule { max-height »%1; }", Kind::Integer, 0, 65535},
    {"window-rule { opacity »%1; }", Kind::Number, 0, 1},
    {"window-rule { geometry-corner-radius »%1; }", Kind::Number, 0, 65535},
    {"window-rule { geometry-corner-radius 1 2 3 »%1; }", Kind::Number, 0, 65535},
    {"window-rule { default-floating-position x=»%1 y=0; }", Kind::Number, -65535, 65535},
    {"window-rule { default-floating-position x=0 y=»%1; }", Kind::Number, -65535, 65535},
    {"window-rule { default-column-width { proportion »%1; }; }", Kind::Positive, 0, 1},
    {"window-rule { default-column-width { fixed »%1; }; }", Kind::Integer, 1, 65535},
    {"window-rule { default-window-height { proportion »%1; }; }", Kind::Positive, 0, 1},
    {"window-rule { default-window-height { fixed »%1; }; }", Kind::Integer, 1, 65535},
    {"window-rule { focus-ring { width »%1; }; }", Kind::Number, 0, 65535},
    {"monitor-profile \"p\" { match aspect-ratio-above=»%1; }", Kind::Number, 0, 100000},
    {"monitor-profile \"p\" { match aspect-ratio-below=»%1; }", Kind::Number, 0, 100000},
    {"monitor-profile \"p\" { match width-above=»%1; }", Kind::Number, 0, 100000},
    {"monitor-profile \"p\" { match width-below=»%1; }", Kind::Number, 0, 100000},
    {"monitor-profile \"p\" { match height-above=»%1; }", Kind::Number, 0, 100000},
    {"monitor-profile \"p\" { match height-below=»%1; }", Kind::Number, 0, 100000},
    {"output \"DP-1\" { layout { gaps »%1; }; }", Kind::Number, 0, 65535},
    {"workspace \"w\" { layout { gaps »%1; }; }", Kind::Number, 0, 65535},
    {"binds { Mod+A cooldown-ms=»%1 { close-window; }; }", Kind::Integer, 0, kIntMax},
};

QString withValue(const RangeCase &range, const QString &value)
{
    return QString::fromUtf8(range.text).arg(value);
}

QString betweenMessage(const RangeCase &range)
{
    if (range.kind == Kind::Positive) {
        return QStringLiteral("value must be greater than 0 and at most %1").arg(range.high);
    }
    return QStringLiteral("value must be between %1 and %2").arg(range.low).arg(range.high);
}

QString belowLow(const RangeCase &range)
{
    return range.kind == Kind::Positive ? QStringLiteral("0") : QString::number(range.low - 1);
}

QString rowName(const RangeCase &range, const char *suffix)
{
    return QString::fromUtf8(range.text).remove(u'»').replace(QStringLiteral("%1"), QString::fromLatin1(suffix));
}

}

class TestConfigRanges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void acceptsBoundaries_data();
    void acceptsBoundaries();
    void rejectsOutsideBoundaries_data();
    void rejectsOutsideBoundaries();
    void integersRejectFractions_data();
    void integersRejectFractions();
    void rejectsNonNumbers();
    void boundaryValuesAreStored();
    void springLimits();
    void monitorMatchBoundsMustLeaveRoom_data();
    void monitorMatchBoundsMustLeaveRoom();
    void ruleMaximumMustNotUndercutMinimum();
};

void TestConfigRanges::acceptsBoundaries_data()
{
    QTest::addColumn<QString>("text");
    for (const RangeCase &range : kCases) {
        const QString low = range.kind == Kind::Positive ? QStringLiteral("0.001") : QString::number(range.low);
        QTest::newRow(qPrintable(rowName(range, "low"))) << unmarked(withValue(range, low));
        QTest::newRow(qPrintable(rowName(range, "high"))) << unmarked(withValue(range, QString::number(range.high)));
    }
}

void TestConfigRanges::acceptsBoundaries()
{
    QFETCH(QString, text);
    verifyLoads(text);
}

void TestConfigRanges::rejectsOutsideBoundaries_data()
{
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");
    for (const RangeCase &range : kCases) {
        const QString message = betweenMessage(range);
        QTest::newRow(qPrintable(rowName(range, "below"))) << withValue(range, belowLow(range)) << message;
        QTest::newRow(qPrintable(rowName(range, "above"))) << withValue(range, QString::number(range.high + 1)) << message;
        if (range.kind != Kind::Integer) {
            QTest::newRow(qPrintable(rowName(range, "fraction above")))
                << withValue(range, QString::number(static_cast<double>(range.high) + 0.5, 'f', 1)) << message;
        }
    }
}

void TestConfigRanges::rejectsOutsideBoundaries()
{
    QFETCH(QString, marked);
    QFETCH(QString, message);
    verifyFailure(marked, message);
}

void TestConfigRanges::integersRejectFractions_data()
{
    QTest::addColumn<QString>("marked");
    for (const RangeCase &range : kCases) {
        if (range.kind == Kind::Integer) {
            QTest::newRow(qPrintable(rowName(range, "1.5"))) << withValue(range, QStringLiteral("1.5"));
        }
    }
}

void TestConfigRanges::integersRejectFractions()
{
    QFETCH(QString, marked);
    verifyFailure(marked, QStringLiteral("expected an integer"));
}

void TestConfigRanges::rejectsNonNumbers()
{
    verifyFailure(QStringLiteral("layout { gaps »\"8\"; }"), QStringLiteral("unsupported value, only numbers are recognized"));
    verifyFailure(QStringLiteral("layout { gaps »true; }"), QStringLiteral("unsupported value, only numbers are recognized"));
    verifyFailure(QStringLiteral("window-rule { opacity »null; }"), QStringLiteral("unsupported value, only numbers are recognized"));
    verifyFailure(QStringLiteral("animations { slowdown »\"2\"; }"), QStringLiteral("unsupported value, only numbers are recognized"));
    verifyFailure(QStringLiteral("layout { max-rows-per-column »\"2\"; }"), QStringLiteral("expected an integer"));
    verifyFailure(QStringLiteral("layout { preset-column-widths { fixed »\"800\"; }; }"), QStringLiteral("expected an integer"));
    verifyFailure(QStringLiteral("layout { preset-column-widths { »proportion; }; }"),
        QStringLiteral("additional argument `proportion` is required"));
    verifyFailure(QStringLiteral("layout { preset-column-widths { proportion 0.5 »0.25; }; }"), QStringLiteral("unexpected argument"));
}

void TestConfigRanges::boundaryValuesAreStored()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            preset-column-widths { proportion 1.0; fixed 1; }
            default-column-width { proportion 0.001; }
            tab-indicator { length total-proportion=0; }
        }
        animations { slowdown 0.001; window-open { duration-ms 1; }; }
        gestures { dnd-edge-view-scroll { max-speed 1; }; }
        window-rule { opacity 0; min-width 400; max-width 400; min-height 500; max-height 0; }
    )"));
    QCOMPARE(proportionOf(config.layout.presetColumnWidths.at(0)), 1.0);
    QCOMPARE(fixedOf(config.layout.presetColumnWidths.at(1)), 1.0);
    QCOMPARE(proportionOf(*config.layout.defaultColumnWidth), 0.001);
    QCOMPARE(config.layout.tabIndicator.lengthTotalProportion, 0.0);
    QCOMPARE(config.animations.slowdown, 0.001);
    QCOMPARE(std::get<EasingParams>(config.animations.windowOpen.kind).durationMs, 1.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.maxSpeed, 1.0);
    const WindowRule &rule = config.windowRules.first();
    QCOMPARE(rule.opacity, std::optional(0.0));
    QCOMPARE(rule.maxWidth, std::optional(400));
    QCOMPARE(rule.maxHeight, std::optional(0));
}

void TestConfigRanges::springLimits()
{
    const QString spring
        = QStringLiteral("animations {\n    window-movement {\n        »spring damping-ratio=%1 stiffness=800 epsilon=%2\n    }\n}\n");
    verifyLoads(unmarked(spring.arg(QStringLiteral("0.1"), QStringLiteral("0.00001"))));
    verifyLoads(unmarked(spring.arg(QStringLiteral("10.0"), QStringLiteral("0.1"))));
    verifyFailure(
        spring.arg(QStringLiteral("0.09"), QStringLiteral("0.001")), QStringLiteral("damping-ratio must be between 0.1 and 10.0"));
    verifyFailure(
        spring.arg(QStringLiteral("10.5"), QStringLiteral("0.001")), QStringLiteral("damping-ratio must be between 0.1 and 10.0"));
    verifyFailure(spring.arg(QStringLiteral("1.0"), QStringLiteral("0.000001")), QStringLiteral("epsilon must be between 0.00001 and 0.1"));
    verifyFailure(spring.arg(QStringLiteral("1.0"), QStringLiteral("0.2")), QStringLiteral("epsilon must be between 0.00001 and 0.1"));
}

void TestConfigRanges::monitorMatchBoundsMustLeaveRoom_data()
{
    QTest::addColumn<QString>("name");
    QTest::newRow("aspect ratio") << QStringLiteral("aspect-ratio");
    QTest::newRow("width") << QStringLiteral("width");
    QTest::newRow("height") << QStringLiteral("height");
}

void TestConfigRanges::monitorMatchBoundsMustLeaveRoom()
{
    QFETCH(QString, name);
    const QString match = QStringLiteral("monitor-profile \"p\" {\n    match %1-below=%2 %1-above=»%3\n}\n");
    const QString message = QStringLiteral("`%1-above` must be less than `%1-below`").arg(name);
    verifyLoads(unmarked(match.arg(name, QStringLiteral("2"), QStringLiteral("1.5"))));
    verifyFailure(match.arg(name, QStringLiteral("2"), QStringLiteral("2")), message);
    verifyFailure(match.arg(name, QStringLiteral("1"), QStringLiteral("3")), message);
    verifyLoads(QStringLiteral("monitor-profile \"p\" {\n    match %1-above=3\n    match %1-below=1\n}\n").arg(name));
}

void TestConfigRanges::ruleMaximumMustNotUndercutMinimum()
{
    verifyFailure(QStringLiteral("window-rule {\n    min-width 800\n    »max-width 400\n}\n"),
        QStringLiteral("`max-width` must be 0 or at least `min-width`"));
    verifyFailure(QStringLiteral("window-rule {\n    »max-height 10\n    min-height 11\n}\n"),
        QStringLiteral("`max-height` must be 0 or at least `min-height`"));
    verifyLoads(QStringLiteral("window-rule { min-width 800; max-width 0; }\nwindow-rule { max-width 400; }\n"));
}

QTEST_MAIN(TestConfigRanges)
#include "test_config_ranges.moc"
