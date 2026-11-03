#include "render/borderimage.h"
#include "render/colormix.h"

#include <QTest>

using namespace Konveyor;

namespace
{

Render::Rgba mix(QColor from, QColor to, double ratio, Config::GradientInterpolation space,
    Config::HueInterpolation hue = Config::HueInterpolation::Shorter)
{
    return Render::mixColors(Render::fromQColor(from), Render::fromQColor(to), ratio, space, hue);
}

void compareRgba(const Render::Rgba &actual, const Render::Rgba &expected, double tolerance)
{
    QVERIFY2(std::abs(actual.r - expected.r) < tolerance, qPrintable(QStringLiteral("r %1 vs %2").arg(actual.r).arg(expected.r)));
    QVERIFY2(std::abs(actual.g - expected.g) < tolerance, qPrintable(QStringLiteral("g %1 vs %2").arg(actual.g).arg(expected.g)));
    QVERIFY2(std::abs(actual.b - expected.b) < tolerance, qPrintable(QStringLiteral("b %1 vs %2").arg(actual.b).arg(expected.b)));
    QVERIFY2(std::abs(actual.a - expected.a) < tolerance, qPrintable(QStringLiteral("a %1 vs %2").arg(actual.a).arg(expected.a)));
}

enum HueResult
{
    Magenta,
    Green,
    Orange,
    Blue
};

void addSpaceRows()
{
    QTest::addColumn<int>("space");
    QTest::newRow("srgb") << static_cast<int>(Config::GradientInterpolation::Srgb);
    QTest::newRow("srgb-linear") << static_cast<int>(Config::GradientInterpolation::SrgbLinear);
    QTest::newRow("oklab") << static_cast<int>(Config::GradientInterpolation::Oklab);
    QTest::newRow("oklch") << static_cast<int>(Config::GradientInterpolation::Oklch);
}

QColor pixel(const QImage &image, int x, int y)
{
    return QColor::fromRgba(qUnpremultiply(image.pixel(x, y)));
}

}

class TestRender : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void srgbMixIsLinearInPremultipliedSpace()
    {
        compareRgba(mix(Qt::red, Qt::blue, 0.5, Config::GradientInterpolation::Srgb), {0.5, 0, 0.5, 1}, 1e-9);
    }

    void allSpacesKeepEndpoints_data() { addSpaceRows(); }

    void allSpacesKeepEndpoints()
    {
        QFETCH(int, space);
        const auto interpolation = static_cast<Config::GradientInterpolation>(space);
        const QColor from(40, 120, 200);
        const QColor to(220, 60, 30);
        compareRgba(mix(from, to, 0.0, interpolation), Render::premultiplied(Render::fromQColor(from)), 2e-3);
        compareRgba(mix(from, to, 1.0, interpolation), Render::premultiplied(Render::fromQColor(to)), 2e-3);
    }

    void transparentEndpointsDoNotDarken()
    {
        const Render::Rgba mixed
            = mix(QColor(255, 255, 255, 0), QColor(255, 255, 255, 255), 0.5, Config::GradientInterpolation::SrgbLinear);
        compareRgba(mixed, {0.5, 0.5, 0.5, 0.5}, 2e-3);
    }

    void oklchHueDirections()
    {
        const QColor red(255, 0, 0);
        const QColor blue(0, 0, 255);
        const Render::Rgba shorter = mix(red, blue, 0.5, Config::GradientInterpolation::Oklch, Config::HueInterpolation::Shorter);
        const Render::Rgba longer = mix(red, blue, 0.5, Config::GradientInterpolation::Oklch, Config::HueInterpolation::Longer);
        QVERIFY(shorter.r > shorter.g);
        QVERIFY(longer.g > shorter.g);
    }

    void roundingAlphaIsOneOutsideCorners()
    {
        const Config::CornerRadius radius {10, 10, 10, 10};
        QCOMPARE(Render::cornerCoverage({50, 50}, {100, 100}, radius, 1.0), 1.0);
        QCOMPARE(Render::cornerCoverage({0.5, 0.5}, {100, 100}, radius, 1.0), 0.0);
        QVERIFY(Render::cornerCoverage({10, 10}, {100, 100}, radius, 1.0) > 0.99);
    }

    void solidBorderHasTransparentInterior()
    {
        Render::BorderSpec spec;
        spec.size = {40, 30};
        spec.borderWidth = 4;
        spec.color = QColor(27, 145, 213);
        const QImage image = Render::renderBorder(spec);
        QCOMPARE(image.size(), QSize(40, 30));
        QCOMPARE(pixel(image, 1, 15), spec.color);
        QCOMPARE(pixel(image, 20, 1), spec.color);
        QCOMPARE(pixel(image, 38, 28), spec.color);
        QCOMPARE(qAlpha(image.pixel(20, 15)), 0);
        QCOMPARE(qAlpha(image.pixel(5, 15)), 0);
    }

    void roundedBorderClearsCorners()
    {
        Render::BorderSpec spec;
        spec.size = {60, 60};
        spec.borderWidth = 4;
        spec.outerRadius = {12, 12, 12, 12};
        spec.color = Qt::white;
        const QImage image = Render::renderBorder(spec);
        QCOMPARE(qAlpha(image.pixel(0, 0)), 0);
        QCOMPARE(qAlpha(image.pixel(59, 59)), 0);
        QCOMPARE(qAlpha(image.pixel(30, 1)), 255);
    }

    void scaledBorderUsesDevicePixels()
    {
        Render::BorderSpec spec;
        spec.size = {20, 10};
        spec.borderWidth = 2;
        spec.color = Qt::green;
        spec.scale = 2.0;
        const QImage image = Render::renderBorder(spec);
        QCOMPARE(image.size(), QSize(40, 20));
        QCOMPARE(image.devicePixelRatio(), 2.0);
        QCOMPARE(qAlpha(image.pixel(3, 10)), 255);
        QCOMPARE(qAlpha(image.pixel(5, 10)), 0);
    }

    void horizontalGradientRunsLeftToRight()
    {
        Render::BorderSpec spec;
        spec.size = {100, 20};
        spec.borderWidth = 0;
        spec.gradient = Config::Gradient {Qt::black, Qt::white, 90};
        spec.gradientRect = {0, 0, 100, 20};
        const QImage image = Render::renderBorder(spec);
        QVERIFY(qRed(image.pixel(2, 10)) < 20);
        QVERIFY(qRed(image.pixel(97, 10)) > 235);
        QVERIFY(std::abs(qRed(image.pixel(50, 10)) - 128) < 6);
    }

    void verticalGradientDefaultAngleRunsTopToBottom()
    {
        Render::BorderSpec spec;
        spec.size = {20, 100};
        spec.gradient = Config::Gradient {Qt::black, Qt::white};
        spec.gradientRect = {0, 0, 20, 100};
        const QImage image = Render::renderBorder(spec);
        QVERIFY(qRed(image.pixel(10, 2)) < 20);
        QVERIFY(qRed(image.pixel(10, 97)) > 235);
    }

    void gradientAnglePointsFromTheFromColorToTheToColor_data()
    {
        QTest::addColumn<double>("angle");
        QTest::addColumn<QPoint>("dark");
        QTest::addColumn<QPoint>("bright");
        QTest::newRow("0 runs bottom to top") << 0.0 << QPoint(50, 97) << QPoint(50, 2);
        QTest::newRow("45 runs to the top right") << 45.0 << QPoint(2, 97) << QPoint(97, 2);
        QTest::newRow("135 runs to the bottom right") << 135.0 << QPoint(2, 2) << QPoint(97, 97);
        QTest::newRow("225 runs to the bottom left") << 225.0 << QPoint(97, 2) << QPoint(2, 97);
        QTest::newRow("270 runs right to left") << 270.0 << QPoint(97, 50) << QPoint(2, 50);
        QTest::newRow("315 runs to the top left") << 315.0 << QPoint(97, 97) << QPoint(2, 2);
    }

    void gradientAnglePointsFromTheFromColorToTheToColor()
    {
        QFETCH(double, angle);
        QFETCH(QPoint, dark);
        QFETCH(QPoint, bright);
        Render::BorderSpec spec;
        spec.size = {100, 100};
        spec.gradient = Config::Gradient {Qt::black, Qt::white, angle};
        spec.gradientRect = {0, 0, 100, 100};
        const QImage image = Render::renderBorder(spec);
        QVERIFY2(qRed(image.pixel(dark)) < 20, qPrintable(QString::number(qRed(image.pixel(dark)))));
        QVERIFY2(qRed(image.pixel(bright)) > 235, qPrintable(QString::number(qRed(image.pixel(bright)))));
    }

    void gradientOverAnEmptyRectStaysOnTheFromColor()
    {
        Render::BorderSpec spec;
        spec.size = {20, 20};
        spec.borderWidth = 5;
        spec.gradient = Config::Gradient {Qt::black, Qt::white, 90};
        const QImage image = Render::renderBorder(spec);
        QCOMPARE(qRed(image.pixel(2, 10)), 0);
        QCOMPARE(qRed(image.pixel(17, 10)), 0);
        QCOMPARE(qAlpha(image.pixel(2, 10)), 255);
    }

    void gradientRelativeToTheWorkspaceViewShowsTheWindowsSlice()
    {
        Render::BorderSpec spec;
        spec.size = {100, 20};
        spec.gradient = Config::Gradient {Qt::black, Qt::white, 90};
        spec.gradientRect = {0, 0, 200, 20};
        spec.geometryOffset = {100, 0};
        const QImage image = Render::renderBorder(spec);
        QVERIFY(std::abs(qRed(image.pixel(0, 10)) - 128) < 6);
        QVERIFY(qRed(image.pixel(99, 10)) > 250);
    }

    void hueModesChooseTheirDirection_data()
    {
        QTest::addColumn<QColor>("from");
        QTest::addColumn<QColor>("to");
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("expected");
        const QColor red(255, 0, 0);
        const QColor blue(0, 0, 255);
        const QColor yellow(255, 255, 0);
        const auto row = [](const char *name, QColor from, QColor to, Config::HueInterpolation mode, int expected) {
            QTest::newRow(name) << from << to << static_cast<int>(mode) << expected;
        };
        using Hue = Config::HueInterpolation;
        row("red to blue shorter", red, blue, Hue::Shorter, Magenta);
        row("red to blue longer", red, blue, Hue::Longer, Green);
        row("red to blue increasing", red, blue, Hue::Increasing, Green);
        row("red to blue decreasing", red, blue, Hue::Decreasing, Magenta);
        row("blue to red shorter", blue, red, Hue::Shorter, Magenta);
        row("blue to red longer", blue, red, Hue::Longer, Green);
        row("blue to red increasing", blue, red, Hue::Increasing, Magenta);
        row("blue to red decreasing", blue, red, Hue::Decreasing, Green);
        row("red to yellow shorter", red, yellow, Hue::Shorter, Orange);
        row("red to yellow longer", red, yellow, Hue::Longer, Blue);
        row("red to yellow increasing", red, yellow, Hue::Increasing, Orange);
        row("red to yellow decreasing", red, yellow, Hue::Decreasing, Blue);
        row("yellow to red shorter", yellow, red, Hue::Shorter, Orange);
        row("yellow to red longer", yellow, red, Hue::Longer, Blue);
        row("yellow to red increasing", yellow, red, Hue::Increasing, Blue);
        row("yellow to red decreasing", yellow, red, Hue::Decreasing, Orange);
    }

    void hueModesChooseTheirDirection()
    {
        QFETCH(QColor, from);
        QFETCH(QColor, to);
        QFETCH(int, mode);
        QFETCH(int, expected);
        const Render::Rgba mixed = mix(from, to, 0.5, Config::GradientInterpolation::Oklch, static_cast<Config::HueInterpolation>(mode));
        switch (expected) {
        case Magenta:
            QVERIFY(mixed.g < mixed.r && mixed.g < mixed.b);
            break;
        case Green:
            QVERIFY(mixed.g > mixed.r && mixed.g > mixed.b);
            break;
        case Orange:
            QVERIFY(mixed.r > mixed.g && mixed.g > mixed.b);
            break;
        default:
            QVERIFY(mixed.b > mixed.r && mixed.b > mixed.g);
            break;
        }
    }

    void oklabMixesBetweenTheEndpoints()
    {
        const Render::Rgba mixed = mix(Qt::red, Qt::blue, 0.5, Config::GradientInterpolation::Oklab);
        QVERIFY(mixed.r > 0.1 && mixed.b > 0.1);
        QVERIFY(mixed.g < mixed.r);
        QCOMPARE(mixed.a, 1.0);
    }

    void fullyTransparentMixesStayTransparent_data() { addSpaceRows(); }

    void fullyTransparentMixesStayTransparent()
    {
        QFETCH(int, space);
        const Render::Rgba mixed = mix(QColor(255, 0, 0, 0), QColor(0, 0, 255, 0), 0.5, static_cast<Config::GradientInterpolation>(space));
        compareRgba(mixed, {0, 0, 0, 0}, 1e-9);
    }

    void pixelsClampAndRound()
    {
        QCOMPARE(Render::toPremultipliedPixel({2.0, -1.0, 0.5, 1.0}), qRgba(255, 0, 128, 255));
        QCOMPARE(Render::toPremultipliedPixel({0.0, 0.0, 0.0, 0.0}), qRgba(0, 0, 0, 0));
    }

    void premultiplyingScalesTheColorByItsAlpha()
    {
        compareRgba(Render::premultiplied({1.0, 0.5, 0.25, 0.5}), {0.5, 0.25, 0.125, 0.5}, 1e-12);
        compareRgba(Render::fromQColor(QColor(255, 0, 51, 255)), {1.0, 0.0, 0.2, 1.0}, 1e-4);
    }
};

QTEST_GUILESS_MAIN(TestRender)

#include "test_render.moc"
