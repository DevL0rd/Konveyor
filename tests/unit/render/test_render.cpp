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

    void allSpacesKeepEndpoints_data()
    {
        QTest::addColumn<int>("space");
        QTest::newRow("srgb") << static_cast<int>(Config::GradientInterpolation::Srgb);
        QTest::newRow("srgb-linear") << static_cast<int>(Config::GradientInterpolation::SrgbLinear);
        QTest::newRow("oklab") << static_cast<int>(Config::GradientInterpolation::Oklab);
        QTest::newRow("oklch") << static_cast<int>(Config::GradientInterpolation::Oklch);
    }

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
};

QTEST_GUILESS_MAIN(TestRender)

#include "test_render.moc"
