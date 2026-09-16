#include "layout/common/geometry.h"
#include "layout/common/sizechange.h"

#include <QTest>

#include <algorithm>

using namespace Konveyor;
using namespace Konveyor::Layout;

namespace
{

bool parksAtHome(QRectF frame, QRectF home, const QList<QRectF> &outputs)
{
    const QRectF parked = parkedFrame(frame, home, outputs);
    const bool offEveryOutput = std::ranges::none_of(outputs, [&](const QRectF &output) { return parked.intersects(output); });
    return parked.size() == frame.size() && offEveryOutput && nearestOutputIndex(parked.center(), outputs) == outputs.indexOf(home);
}

}

class TestLayoutGeometry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void roundsLogicalToPhysical()
    {
        QCOMPARE(snapToPixels(2.0, 1.4), 1.5);
        QCOMPARE(snapToPixelsAtLeastOne(2.0, 0.0), 0.0);
        QCOMPARE(snapToPixelsAtLeastOne(2.0, 0.1), 0.5);
        QCOMPARE(floorToPixelsAtLeastOne(2.0, 0.1), 0.5);
        QCOMPARE(ceilToPixels(2.0, 1.1), 1.5);
        QCOMPARE(roundPoint(QPointF(1.4, 1.4), 2.0), QPointF(1.5, 1.5));
        QCOMPARE(roundSize(QSizeF(1.4, 1.4), 2.0), QSizeF(1.5, 1.5));
    }

    void convertsToIntegers()
    {
        QCOMPARE(saturatingInt(std::nan("")), 0);
        QCOMPARE(floorToInt(1.9), 1);
        QCOMPARE(roundToInt(1.6), 2);
        QCOMPARE(floorSize(QSizeF(1.9, 2.9)), QSize(1, 2));
        QCOMPARE(roundedSize(QSizeF(1.6, 2.4)), QSize(2, 2));
    }

    void computesWorkingArea()
    {
        Config::Struts struts {10, 20, 30, 40};
        const QRectF area = workAreaWithStruts(QRectF(0, 0, 1000, 800), 1.0, struts);
        QCOMPARE(area, QRectF(10, 30, 970, 730));
    }

    void workingAreaClampsToZero()
    {
        Config::Struts struts {2000, 2000, 2000, 2000};
        const QRectF area = workAreaWithStruts(QRectF(0, 0, 1000, 800), 1.0, struts);
        QCOMPARE(area.width(), 0.0);
        QCOMPARE(area.height(), 0.0);
    }

    void computesViewOffsetForFittingColumn() { QCOMPARE(scrollToReveal(0.0, 1920.0, 0.0, 936.0, 16.0), -16.0); }

    void computesViewOffsetForWideColumn() { QCOMPARE(scrollToReveal(0.0, 100.0, 0.0, 200.0, 16.0), 0.0); }

    void computesViewOffsetSnappingRight()
    {
        const double offset = scrollToReveal(0.0, 1000.0, 5000.0, 500.0, 16.0);
        QCOMPARE(offset, -(1000.0 - 16.0 - 500.0));
    }

    void keepsVisibleColumnInPlace() { QCOMPARE(scrollToReveal(0.0, 1000.0, 100.0, 500.0, 16.0), -100.0); }

    void ensuresMinMaxSizes()
    {
        QCOMPARE(clampToSizeLimits(100, 200, 0), 200);
        QCOMPARE(clampToSizeLimits(500, 0, 300), 300);
        QCOMPARE(clampToSizeLimitsAllowZero(0, 200, 200), 200);
        QCOMPARE(clampToSizeLimitsAllowZero(0, 100, 200), 0);
        QCOMPARE(clampToSizeLimitsAllowZero(50, 100, 200), 100);
    }

    void clampsAndCentersInArea()
    {
        const QRectF area(0, 0, 100, 100);
        QCOMPARE(clampIntoArea(area, QRectF(200, 200, 40, 40)), QPointF(60, 60));
        QCOMPARE(clampIntoArea(area, QRectF(-50, -50, 200, 200)), QPointF(0, 0));
        QCOMPARE(centerInArea(area, QSizeF(40, 60)), QPointF(30, 20));
        QCOMPARE(centerInArea(area, QSizeF(400, 600)), QPointF(0, 0));
    }

    void resolvesPresetSizes()
    {
        Options options;
        options.layout.gaps = 16;
        const PresetExtent proportional = measurePreset(Config::Proportion {0.5}, options, 1920, 0);
        QVERIFY(proportional.isTile);
        QCOMPARE(proportional.value, 936.0);
        const PresetExtent fixed = measurePreset(Config::Fixed {300.4}, options, 1920, 0);
        QVERIFY(!fixed.isTile);
        QCOMPARE(fixed.value, 300.0);
        const PresetExtent floating = measureFloatingPreset(Config::Proportion {0.5}, 1000);
        QVERIFY(floating.isTile);
        QCOMPARE(floating.value, 500.0);
    }

    void computesBorderExtentAndBounds()
    {
        Config::Border border;
        border.enabled = true;
        border.width = 4;
        QCOMPARE(borderExtent(border), 8.0);
        QCOMPARE(maxWindowBounds(border, QSizeF(1000, 800), QSizeF(0, 0), 16.0), QSize(960, 760));
    }

    void columnWidthFromPreset()
    {
        QCOMPARE(ColumnWidth::fromPreset(Config::Proportion {0.25}), ColumnWidth::proportion(0.25));
        QCOMPARE(ColumnWidth::fromPreset(Config::Fixed {300.6}), ColumnWidth::fixed(301.0));
    }

    void mapsSmartActivation()
    {
        QVERIFY(resolveActivation(Activation::Always, false));
        QVERIFY(!resolveActivation(Activation::Never, true));
        QVERIFY(resolveActivation(Activation::Smart, true));
        QVERIFY(!resolveActivation(Activation::Smart, false));
    }

    void parsesSizeChanges()
    {
        const auto fixed = parseSizeChange(QStringLiteral("100"));
        QVERIFY(fixed.has_value());
        QCOMPARE(fixed->kind, ChangeKind::SetFixed);
        QCOMPARE(fixed->value, 100.0);

        const auto proportion = parseSizeChange(QStringLiteral("50%"));
        QVERIFY(proportion.has_value());
        QCOMPARE(proportion->kind, ChangeKind::SetProportion);
        QCOMPARE(proportion->value, 50.0);

        const auto adjust = parseSizeChange(QStringLiteral("+10"));
        QVERIFY(adjust.has_value());
        QCOMPARE(adjust->kind, ChangeKind::AdjustFixed);

        const auto adjustProportion = parseSizeChange(QStringLiteral("-10%"));
        QVERIFY(adjustProportion.has_value());
        QCOMPARE(adjustProportion->kind, ChangeKind::AdjustProportion);
        QCOMPARE(adjustProportion->value, -10.0);
    }

    void rejectsBadSizeChanges()
    {
        QVERIFY(!parseSizeChange(QStringLiteral("10%x")).has_value());
        QVERIFY(!parseSizeChange(QStringLiteral("%")).has_value());
        QVERIFY(!parseSizeChange(QStringLiteral("abc")).has_value());
    }

    void parsesPositionChanges()
    {
        const auto value = parsePositionChange(QStringLiteral("12.5"));
        QVERIFY(value.has_value());
        QCOMPARE(value->kind, ChangeKind::SetFixed);
        QCOMPARE(value->value, 12.5);
        QVERIFY(parsePositionChange(QStringLiteral("+1.5")).value().kind == ChangeKind::AdjustFixed);
    }

    void parsesWorkspaceReferences()
    {
        const auto index = parseWorkspaceReference(QStringLiteral("3"));
        QVERIFY(index.has_value());
        QCOMPARE(index->kind, Config::WorkspaceReferenceKind::Index);
        QCOMPARE(index->index, 3u);

        const auto named = parseWorkspaceReference(QStringLiteral("browser"));
        QVERIFY(named.has_value());
        QCOMPARE(named->kind, Config::WorkspaceReferenceKind::Name);
        QCOMPARE(named->name, QStringLiteral("browser"));

        QVERIFY(!parseWorkspaceReference(QStringLiteral("999")).has_value());
    }

    void parsesEnumsAndBooleans()
    {
        QCOMPARE(parseColumnDisplay(QStringLiteral("tabbed")).value(), Config::ColumnDisplay::Tabbed);
        QVERIFY(!parseColumnDisplay(QStringLiteral("grid")).has_value());
        QCOMPARE(parseBool(QStringLiteral("#true")).value(), true);
        QCOMPARE(parseBool(QStringLiteral("false")).value(), false);
        QVERIFY(!parseBool(QStringLiteral("maybe")).has_value());
        QCOMPARE(parseIndex(QStringLiteral("7")).value(), 7u);
        QVERIFY(!parseIndex(QStringLiteral("x")).has_value());
    }

    void sizeChangeFromPresetValues()
    {
        QCOMPARE(sizeChangeFromPreset(Config::Proportion {0.25}).kind, ChangeKind::SetProportion);
        QCOMPARE(sizeChangeFromPreset(Config::Proportion {0.25}).value, 25.0);
        QCOMPARE(sizeChangeFromPreset(Config::Fixed {100.4}).kind, ChangeKind::SetFixed);
        QCOMPARE(sizeChangeFromPreset(Config::Fixed {100.4}).value, 100.0);
    }

    void nearestOutputMatchesKWin()
    {
        const QList<QRectF> outputs {QRectF(0, 0, 5120, 1440), QRectF(-2328, 0, 2328, 1600)};
        QCOMPARE(nearestOutputIndex(QPointF(100, 100), outputs), 0);
        QCOMPARE(nearestOutputIndex(QPointF(-100, 100), outputs), 1);
        QCOMPARE(nearestOutputIndex(QPointF(-100, 1500), outputs), 1);
        QCOMPARE(nearestOutputIndex(QPointF(9000, 100), outputs), 0);
    }

    void parkedFrameStaysOffEveryOutput()
    {
        const QList<QRectF> outputs {QRectF(0, 0, 5120, 1440), QRectF(-2560, 400, 2560, 1440)};
        for (const QRectF &home : outputs) {
            QVERIFY(parksAtHome(QRectF(-1300, 58, 1260, 1366), home, outputs));
        }
    }

    void parkedFrameKeepsHomeInARowOfThree()
    {
        const QList<QRectF> outputs {QRectF(0, 0, 1920, 1080), QRectF(1920, 0, 1920, 1080), QRectF(3840, 0, 1920, 1080)};
        for (const QRectF &home : outputs) {
            QVERIFY(parksAtHome(QRectF(-900, 0, 800, 1000), home, outputs));
        }
    }

    void parkedFrameKeepsHomeInAColumn()
    {
        const QList<QRectF> outputs {QRectF(0, 0, 2560, 1440), QRectF(0, 1440, 2560, 1440), QRectF(0, 2880, 2560, 1440)};
        for (const QRectF &home : outputs) {
            QVERIFY(parksAtHome(QRectF(2600, 100, 1200, 1300), home, outputs));
        }
    }
};

QTEST_GUILESS_MAIN(TestLayoutGeometry)
#include "test_layout_geometry.moc"
