#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config centeringConfig(Config::CenterFocusedColumn mode, double width)
{
    Config::Config config = instantConfig();
    config.layout.centerFocusedColumn = mode;
    config.layout.defaultColumnWidth = Config::Fixed {width};
    return config;
}

QList<Layout::WindowId> addColumns(Fixture &fixture, int count)
{
    QList<Layout::WindowId> ids;
    for (int i = 0; i < count; ++i) {
        ids.append(fixture.add(QStringLiteral("column")));
    }
    return ids;
}

}

class TestLayoutCentering : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void onOverflowKeepsTheViewWhileTheNeighbourFits()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::OnOverflow, 600));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        QCOMPARE(fixture.frame(row[3]).x(), 1920.0 - 16.0 - 600.0);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[3]).x(), 1920.0 - 16.0 - 600.0);
        QCOMPARE(fixture.frame(row[2]).x(), 1920.0 - 16.0 - 1216.0);
        VERIFY_INVARIANTS(fixture);
    }

    void onOverflowCentersWhenTheNeighbourDoesNotFit()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::OnOverflow, 1000));
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[1]).x(), 460.0);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[0]).x(), 16.0);
        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.frame(row[1]).x(), 460.0);
        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.frame(row[2]).x(), 904.0);
        VERIFY_INVARIANTS(fixture);
    }

    void onOverflowCountsTheBorderWhenTheNeighbourWouldOnlyFitWithoutIt()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::OnOverflow, 930);
        config.layout.border.enabled = true;
        config.layout.border.width = 8;
        Fixture fixture(config);
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        QCOMPARE(fixture.frame(row[2]).x(), 1920.0 - 16.0 - 8.0 - 930.0);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[1]).x(), (1920.0 - 946.0) / 2.0 + 8.0);
        VERIFY_INVARIANTS(fixture);
    }

    void neverModeOnlyRevealsTheColumn()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::Never, 1000));
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[1]).x(), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void neverModeRevealsTheColumnInsideTheStruts()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Never, 1000);
        config.layout.struts = Config::Struts {100, 200, 0, 0};
        Fixture fixture(config);
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        QCOMPARE(fixture.frame(row[2]).x(), 1920.0 - 200.0 - 16.0 - 1000.0);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[1]).x(), 116.0);
        VERIFY_INVARIANTS(fixture);
    }

    void onOverflowCentersInsideTheStruts()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::OnOverflow, 1000);
        config.layout.struts = Config::Struts {100, 0, 0, 0};
        Fixture fixture(config);
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[1]).x(), 100.0 + (1820.0 - 1000.0) / 2.0);
        fixture.perform(QStringLiteral("focus-column-first"));
        QCOMPARE(fixture.frame(row[0]).x(), 116.0);
        VERIFY_INVARIANTS(fixture);
    }

    void onOverflowAloneStaysAtTheStart()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::OnOverflow, 600));
        const auto a = fixture.add();
        QCOMPARE(fixture.frame(a).x(), 16.0);
    }

    void alwaysCentersInsideTheStruts()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Always, 600);
        config.layout.struts = Config::Struts {200, 0, 30, 0};
        Fixture fixture(config);
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        QCOMPARE(fixture.frame(row[2]), QRectF(200.0 + (1720.0 - 600.0) / 2.0, 46.0, 600.0, 1018.0));
        fixture.perform(QStringLiteral("focus-column-first"));
        QCOMPARE(fixture.frame(row[0]).x(), 760.0);
        VERIFY_INVARIANTS(fixture);
    }

    void alwaysCentersTheTileIncludingItsBorder()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Always, 600);
        config.layout.border.enabled = true;
        config.layout.border.width = 8;
        Fixture fixture(config);
        const auto a = fixture.add();
        QCOMPARE(fixture.frame(a), QRectF((1920.0 - 616.0) / 2.0 + 8.0, 24.0, 600.0, 1032.0));
    }

    void alwaysWithExpandSingleColumnFillsThenCenters()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Always, 600);
        config.layout.alwaysExpandSingleColumn = true;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a), QRectF(16, 16, 1888, 1048));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(a).width(), 600.0);
        QCOMPARE(fixture.frame(b).x(), 660.0);
        fixture.remove(b);
        QCOMPARE(fixture.frame(a), QRectF(16, 16, 1888, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void centerSingleWithExpandSingleUsesTheWorkingArea()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Never, 600);
        config.layout.alwaysCenterSingleColumn = true;
        config.layout.alwaysExpandSingleColumn = true;
        config.layout.struts = Config::Struts {100, 0, 0, 0};
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a), QRectF(120, 20, 1780, 1040));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(a), QRectF(120, 20, 600, 1040));
        QCOMPARE(fixture.frame(b).x(), 120.0 + 600.0 + 4.0 + 16.0 + 4.0);
        VERIFY_INVARIANTS(fixture);
    }

    void centerSingleWithoutExpandCentersInsideTheStruts()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Never, 600);
        config.layout.alwaysCenterSingleColumn = true;
        config.layout.struts = Config::Struts {300, 100, 0, 0};
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a), QRectF(300.0 + (1520.0 - 600.0) / 2.0, 16, 600, 1048));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(a).x(), 316.0);
        QCOMPARE(fixture.frame(b).x(), 932.0);
        fixture.remove(b);
        QCOMPARE(fixture.frame(a), QRectF(300.0 + (1520.0 - 600.0) / 2.0, 16, 600, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void centerSingleKeepsANonResizableAppCentered()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Never, 600);
        config.layout.alwaysCenterSingleColumn = true;
        config.layout.alwaysExpandSingleColumn = true;
        Fixture fixture(config);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("fixed"), QStringLiteral("fixed"), QSizeF(400, 300));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);
        QCOMPARE(fixture.frame(id).size(), QSizeF(400, 300));
        QCOMPARE(fixture.frame(id).x(), 760.0);
    }

    void centerSingleColumnFollowsTheSettingLive()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Never, 600);
        Fixture fixture(config);
        const auto a = fixture.add();
        QCOMPARE(fixture.frame(a).x(), 16.0);
        config.layout.alwaysCenterSingleColumn = true;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(a).x(), 660.0);
        config.layout.alwaysCenterSingleColumn = false;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(a).x(), 16.0);
    }

    void centerModeChangesLive()
    {
        Config::Config config = centeringConfig(Config::CenterFocusedColumn::Never, 600);
        Fixture fixture(config);
        const QList<Layout::WindowId> row = addColumns(fixture, 2);
        QCOMPARE(fixture.frame(row[1]).x(), 632.0);
        config.layout.centerFocusedColumn = Config::CenterFocusedColumn::Always;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(row[1]).x(), 660.0);
        QCOMPARE(fixture.frame(row[0]).x(), 44.0);
        config.layout.centerFocusedColumn = Config::CenterFocusedColumn::Never;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(row[1]).x(), 632.0);
    }

    void centerWindowCentersTheActiveTiledColumn()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::Never, 600));
        const QList<Layout::WindowId> row = addColumns(fixture, 5);
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[2]).x(), 72.0);
        fixture.perform(QStringLiteral("center-window"));
        QCOMPARE(fixture.frame(row[2]).x(), 660.0);
        QCOMPARE(fixture.state(row[2]).isFloating, false);
        VERIFY_INVARIANTS(fixture);
    }

    void centerWindowWithAnotherColumnsIdDoesNothing()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::Never, 600));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.perform(QStringLiteral("center-window"), {}, {{QStringLiteral("id"), QString::number(row[3])}});
        QCOMPARE(fixture.frame(row[2]).x(), 688.0);
        QCOMPARE(fixture.focused(), std::optional(row[2]));
    }

    void centerColumnCannotRevealSpaceBeforeTheFirstColumn()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::Never, 600));
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-first"));
        fixture.perform(QStringLiteral("center-column"));
        QCOMPARE(fixture.frame(row[0]).x(), 16.0);
    }

    void centerColumnWithAlwaysCenteringChangesNothing()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::Always, 600));
        const QList<Layout::WindowId> row = addColumns(fixture, 2);
        fixture.perform(QStringLiteral("center-column"));
        QCOMPARE(fixture.frame(row[1]).x(), 660.0);
        fixture.perform(QStringLiteral("center-visible-columns"));
        QCOMPARE(fixture.frame(row[1]).x(), 660.0);
    }

    void alwaysCentersAColumnWiderThanTheScreenAtItsStart()
    {
        Fixture fixture(centeringConfig(Config::CenterFocusedColumn::Always, 2500));
        const QList<Layout::WindowId> row = addColumns(fixture, 2);
        QCOMPARE(fixture.frame(row[1]).x(), 0.0);
        fixture.perform(QStringLiteral("focus-column-left"));
        QCOMPARE(fixture.frame(row[0]).x(), 0.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutCentering)
#include "test_layout_centering.moc"
