#include "configtesthelpers.h"

#include "config/sections.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

Paint activeRingOf(const QString &body)
{
    return parsed(QStringLiteral("layout { focus-ring { %1 }; }").arg(body)).layout.focusRing.active;
}

QColor activeRingColor(const QString &node)
{
    return activeRingOf(node).color;
}

}

class TestConfigColors : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesColorForms_data();
    void parsesColorForms();
    void parsesFourChannelForm();
    void parsesAccentKeyword();
    void rejectsInvalidColors_data();
    void rejectsInvalidColors();
    void parsesGradients();
    void parsesGradientInterpolation_data();
    void parsesGradientInterpolation();
    void rejectsGradientInterpolation_data();
    void rejectsGradientInterpolation();
    void requiresGradientEndpoints();
    void colorClearsGradient();
    void gradientKeepsEarlierColor();
    void windowRuleColorOverridesLayoutGradient();
};

void TestConfigColors::parsesColorForms_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QColor>("expected");

    QTest::newRow("#rgb") << QStringLiteral("#f80") << QColor(0xff, 0x88, 0x00);
    QTest::newRow("#rgba") << QStringLiteral("#0007") << QColor(0, 0, 0, 0x77);
    QTest::newRow("#rrggbb") << QStringLiteral("#7fc8ff") << QColor(127, 200, 255);
    QTest::newRow("#rrggbbaa") << QStringLiteral("#0080ff40") << QColor(0, 128, 255, 64);
    QTest::newRow("name") << QStringLiteral("red") << QColor(255, 0, 0);
    QTest::newRow("name case") << QStringLiteral("CornflowerBlue") << QColor(100, 149, 237);
    QTest::newRow("transparent") << QStringLiteral("transparent") << QColor(0, 0, 0, 0);
    QTest::newRow("rgb") << QStringLiteral("rgb(255, 200, 127)") << QColor(255, 200, 127);
    QTest::newRow("rgb space") << QStringLiteral("rgb(255 200 127)") << QColor(255, 200, 127);
    QTest::newRow("rgba") << QStringLiteral("rgba(25, 25, 102, 1.0)") << QColor(25, 25, 102);
    QTest::newRow("rgb percent") << QStringLiteral("rgb(100%, 0%, 0%)") << QColor(255, 0, 0);
    QTest::newRow("rgb slash alpha") << QStringLiteral("rgb(0 0 0 / 50%)") << QColor(0, 0, 0, 128);
    QTest::newRow("hsl") << QStringLiteral("hsl(0, 100%, 50%)") << QColor(255, 0, 0);
    QTest::newRow("hsl deg") << QStringLiteral("hsl(120deg, 100%, 50%)") << QColor(0, 255, 0);
    QTest::newRow("hsla") << QStringLiteral("hsla(240, 100%, 50%, 0.5)") << QColor(0, 0, 255, 128);
}

void TestConfigColors::parsesColorForms()
{
    QFETCH(QString, text);
    QFETCH(QColor, expected);
    const QColor actual = activeRingColor(QStringLiteral("active-color \"%1\";").arg(text));
    QCOMPARE(actual.red(), expected.red());
    QCOMPARE(actual.green(), expected.green());
    QCOMPARE(actual.blue(), expected.blue());
    QCOMPARE(actual.alpha(), expected.alpha());
}

void TestConfigColors::parsesFourChannelForm()
{
    QCOMPARE(activeRingColor(QStringLiteral("active-color 0 100 200 255;")), QColor(0, 100, 200, 255));
    QCOMPARE(activeRingColor(QStringLiteral("active-color 255 200 100 0;")), QColor(255, 200, 100, 0));
    QVERIFY(
        mustFail(QStringLiteral("layout { focus-ring { active-color 1 2 3; }; }")).message.contains(QStringLiteral("4 color channels")));
    QVERIFY(mustFail(QStringLiteral("layout { focus-ring { active-color 1 2 3 999; }; }"))
            .message.contains(QStringLiteral("between 0 and 255")));
}

void TestConfigColors::parsesAccentKeyword()
{
    const Paint paint = activeRingOf(QStringLiteral("active-color \"accent\";"));
    QCOMPARE(paint.source, ColorSource::SystemAccent);
    QVERIFY(!paint.gradient.has_value());
    QVERIFY(mustFail(QStringLiteral("layout { background-color \"accent\"; }")).message.contains(QStringLiteral("only supported for")));
}

void TestConfigColors::rejectsInvalidColors_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("bad name") << QStringLiteral("notacolor");
    QTest::newRow("short hex") << QStringLiteral("#ff");
    QTest::newRow("long hex") << QStringLiteral("#0011223344");
    QTest::newRow("bad hex digit") << QStringLiteral("#gg0000");
    QTest::newRow("bad function") << QStringLiteral("cmyk(1,2,3,4)");
    QTest::newRow("too few components") << QStringLiteral("rgb(1, 2)");
}

void TestConfigColors::rejectsInvalidColors()
{
    QFETCH(QString, text);
    const LoadError error = mustFail(QStringLiteral("layout { focus-ring { active-color \"%1\"; }; }").arg(text));
    QVERIFY2(error.message.startsWith(QStringLiteral("invalid color: ")), qPrintable(error.message));
}

void TestConfigColors::parsesGradients()
{
    const Paint paint
        = activeRingOf(QStringLiteral("active-gradient from=\"#101010\" to=\"#202020\" angle=45 relative-to=\"workspace-view\";"));
    QVERIFY(paint.gradient.has_value());
    QCOMPARE(paint.gradient->from, QColor(0x10, 0x10, 0x10));
    QCOMPARE(paint.gradient->to, QColor(0x20, 0x20, 0x20));
    QCOMPARE(paint.gradient->angle, 45.0);
    QCOMPARE(paint.gradient->relativeTo, GradientRelativeTo::WorkspaceView);
    QCOMPARE(paint.gradient->interpolation, GradientInterpolation::Srgb);
    QCOMPARE(paint.gradient->hue, HueInterpolation::Shorter);

    const Paint defaults = activeRingOf(QStringLiteral("active-gradient from=\"#000\" to=\"#fff\";"));
    QCOMPARE(defaults.gradient->angle, 180.0);
    QCOMPARE(defaults.gradient->relativeTo, GradientRelativeTo::Window);
}

void TestConfigColors::parsesGradientInterpolation_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("space");
    QTest::addColumn<int>("hue");

    QTest::newRow("srgb") << QStringLiteral("srgb") << int(GradientInterpolation::Srgb) << int(HueInterpolation::Shorter);
    QTest::newRow("srgb-linear") << QStringLiteral("srgb-linear") << int(GradientInterpolation::SrgbLinear)
                                 << int(HueInterpolation::Shorter);
    QTest::newRow("oklab") << QStringLiteral("oklab") << int(GradientInterpolation::Oklab) << int(HueInterpolation::Shorter);
    QTest::newRow("oklch") << QStringLiteral("oklch") << int(GradientInterpolation::Oklch) << int(HueInterpolation::Shorter);
    QTest::newRow("oklch longer") << QStringLiteral("oklch longer hue") << int(GradientInterpolation::Oklch)
                                  << int(HueInterpolation::Longer);
    QTest::newRow("oklch increasing") << QStringLiteral("oklch increasing hue") << int(GradientInterpolation::Oklch)
                                      << int(HueInterpolation::Increasing);
    QTest::newRow("oklch decreasing") << QStringLiteral("oklch decreasing hue") << int(GradientInterpolation::Oklch)
                                      << int(HueInterpolation::Decreasing);
}

void TestConfigColors::parsesGradientInterpolation()
{
    QFETCH(QString, text);
    QFETCH(int, space);
    QFETCH(int, hue);
    const Paint paint = activeRingOf(QStringLiteral("active-gradient from=\"#000\" to=\"#fff\" in=\"%1\";").arg(text));
    QCOMPARE(int(paint.gradient->interpolation), space);
    QCOMPARE(int(paint.gradient->hue), hue);
}

void TestConfigColors::rejectsGradientInterpolation_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("fragment");

    QTest::newRow("empty") << QString() << QStringLiteral("missing color space");
    QTest::newRow("unknown") << QStringLiteral("cmyk") << QStringLiteral("invalid color space");
    QTest::newRow("srgb hue") << QStringLiteral("srgb shorter hue") << QStringLiteral("only oklch");
    QTest::newRow("missing hue") << QStringLiteral("oklch shorter") << QStringLiteral("must end with");
    QTest::newRow("typo hue") << QStringLiteral("oklch shorter h") << QStringLiteral("must end with");
    QTest::newRow("bad mode") << QStringLiteral("oklch sideways hue") << QStringLiteral("invalid hue interpolation");
    QTest::newRow("trailing") << QStringLiteral("oklch shorter hue extra") << QStringLiteral("must end with");
}

void TestConfigColors::rejectsGradientInterpolation()
{
    QFETCH(QString, text);
    QFETCH(QString, fragment);
    const LoadError error
        = mustFail(QStringLiteral("layout { focus-ring { active-gradient from=\"#000\" to=\"#fff\" in=\"%1\"; }; }").arg(text));
    QVERIFY2(error.message.contains(fragment), qPrintable(error.message));
}

void TestConfigColors::requiresGradientEndpoints()
{
    QVERIFY(mustFail(QStringLiteral("layout { focus-ring { active-gradient to=\"#fff\"; }; }"))
            .message.contains(QStringLiteral("`from` is required")));
    QVERIFY(mustFail(QStringLiteral("layout { focus-ring { active-gradient from=\"#fff\"; }; }"))
            .message.contains(QStringLiteral("`to` is required")));
}

void TestConfigColors::colorClearsGradient()
{
    const Paint paint = activeRingOf(QStringLiteral("active-gradient from=\"#000\" to=\"#fff\"; active-color \"#abcdef\";"));
    QVERIFY(!paint.gradient.has_value());
    QCOMPARE(paint.color, QColor(0xab, 0xcd, 0xef));
}

void TestConfigColors::gradientKeepsEarlierColor()
{
    const Paint paint = activeRingOf(QStringLiteral("active-color \"#abcdef\"; active-gradient from=\"#000\" to=\"#fff\";"));
    QVERIFY(paint.gradient.has_value());
    QCOMPARE(paint.color, QColor(0xab, 0xcd, 0xef));
}

void TestConfigColors::windowRuleColorOverridesLayoutGradient()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            border {
                on
                active-gradient from="#101010" to="#202020"
            }
        }
        window-rule {
            border {
                active-color "#abcdef"
            }
        }
    )"));
    QVERIFY(config.layout.border.active.gradient.has_value());
    const BorderRule &rule = config.windowRules.first().border;
    QVERIFY(rule.active.has_value());
    QVERIFY(!rule.active->gradient.has_value());
    QCOMPARE(rule.active->color, QColor(0xab, 0xcd, 0xef));

    Border merged = config.layout.border;
    mergeBorder(merged, rule);
    QVERIFY(!merged.active.gradient.has_value());
    QCOMPARE(merged.active.color, QColor(0xab, 0xcd, 0xef));
}

QTEST_MAIN(TestConfigColors)
#include "test_config_colors.moc"
