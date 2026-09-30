#include "helpers.h"

using namespace LayoutTest;

namespace
{

constexpr double FullHeight = 1080.0 - 2 * 16.0;

struct StackBesideSingle
{
    Fixture fixture;
    Layout::WindowId single = 0;
    Layout::WindowId top = 0;
    Layout::WindowId bottom = 0;

    explicit StackBesideSingle(const Config::Config &config = instantConfig())
        : fixture(config)
    {
        single = fixture.add(QStringLiteral("single"));
        std::tie(top, bottom) = addStackedPair(fixture);
        fixture.engine().activateWindow(top);
        fixture.settle();
    }

    double height(Layout::WindowId id) { return fixture.frame(id).height(); }
};

}

class TestLayoutCarriedHeights : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fixedHeightSurvivesExpelIntoItsOwnColumn()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        QCOMPARE(s.height(s.top), 300.0);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QVERIFY(s.fixture.state(s.top).columnIndex != s.fixture.state(s.bottom).columnIndex);
        QCOMPARE(s.height(s.top), 300.0);
        QCOMPARE(s.height(s.bottom), FullHeight);
        VERIFY_INVARIANTS(s.fixture);
    }

    void fixedHeightSurvivesConsumeIntoAnotherColumn()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.fixture.state(s.top).columnIndex, s.fixture.state(s.single).columnIndex);
        QCOMPARE(s.height(s.top), 300.0);
        QCOMPARE(s.height(s.single), 1080.0 - 3 * 16.0 - 300.0);
        VERIFY_INVARIANTS(s.fixture);
    }

    void heightShareSurvivesAMoveIntoAnotherColumn()
    {
        Fixture fixture;
        const auto single = fixture.add(QStringLiteral("single"));
        const auto [first, second] = addStackedPair(fixture);
        fixture.add(QStringLiteral("third"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(std::abs(fixture.frame(first).height() - (1080.0 - 4 * 16.0) / 3.0) <= 1.0);
        fixture.engine().activateWindow(single);
        fixture.settle();
        QVERIFY(fixture.perform(QStringLiteral("consume-window-into-column")).ok);
        QCOMPARE(fixture.state(first).columnIndex, fixture.state(single).columnIndex);
        const double available = 1080.0 - 3 * 16.0;
        QVERIFY2(
            std::abs(fixture.frame(first).height() - available / 3.0) <= 1.0, qPrintable(QString::number(fixture.frame(first).height())));
        QVERIFY(std::abs(fixture.frame(single).height() - available * 2.0 / 3.0) <= 1.0);
        QCOMPARE(fixture.frame(second).height(), available / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedHeightMeetsAnotherFixedHeight()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        s.fixture.engine().activateWindow(s.single);
        s.fixture.settle();
        const auto extra = s.fixture.add(QStringLiteral("extra"));
        s.fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        s.fixture.engine().activateWindow(s.single);
        s.fixture.settle();
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("200")}).ok);
        QCOMPARE(s.height(s.single), 200.0);
        const double extraBefore = s.height(extra);
        s.fixture.engine().activateWindow(s.top);
        s.fixture.settle();
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.fixture.state(s.top).columnIndex, s.fixture.state(s.single).columnIndex);
        QCOMPARE(s.height(s.top), 300.0);
        const double rest = 1080.0 - 4 * 16.0 - 300.0;
        QVERIFY(std::abs(s.height(s.single) - rest * 200.0 / (200.0 + extraBefore)) <= 1.0);
        VERIFY_INVARIANTS(s.fixture);
    }

    void presetHeightSurvivesAMove()
    {
        Config::Config config = instantConfig();
        config.layout.presetWindowHeights = {Config::PresetSize(Config::Fixed {250}), Config::PresetSize(Config::Fixed {400})};
        StackBesideSingle s(config);
        QVERIFY(s.fixture.perform(QStringLiteral("switch-preset-window-height")).ok);
        QCOMPARE(s.height(s.top), 250.0);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.height(s.top), 250.0);
        QVERIFY(s.fixture.perform(QStringLiteral("switch-preset-window-height")).ok);
        QCOMPARE(s.height(s.top), 400.0);
        VERIFY_INVARIANTS(s.fixture);
    }

    void swappedWindowsTradeTheirHeights()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("swap-window-left")).ok);
        QCOMPARE(s.fixture.state(s.top).columnIndex, 0);
        QCOMPARE(s.height(s.top), 300.0);
        QCOMPARE(s.height(s.single), (1080.0 - 3 * 16.0) / 2.0);
        QCOMPARE(s.height(s.bottom), (1080.0 - 3 * 16.0) / 2.0);
        VERIFY_INVARIANTS(s.fixture);
    }

    void fixedHeightTravelsToAnotherWorkspace()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        const Layout::WorkspaceId before = s.fixture.state(s.top).workspace;
        QVERIFY(s.fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        QVERIFY(s.fixture.state(s.top).workspace != before);
        QCOMPARE(s.height(s.top), 300.0);
        VERIFY_INVARIANTS(s.fixture);
    }

    void tabbedColumnsKeepOneHeight()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        s.fixture.engine().activateWindow(s.single);
        s.fixture.settle();
        QVERIFY(s.fixture.perform(QStringLiteral("toggle-column-tabbed-display")).ok);
        s.fixture.engine().activateWindow(s.top);
        s.fixture.settle();
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.fixture.state(s.top).columnIndex, s.fixture.state(s.single).columnIndex);
        QCOMPARE(s.height(s.top), s.height(s.single));
        VERIFY_INVARIANTS(s.fixture);
    }

    void heightFromATabbedColumnIsNotCarried()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("toggle-column-tabbed-display")).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.height(s.top), FullHeight);
        VERIFY_INVARIANTS(s.fixture);
    }

    void singleWindowFillsItsNewColumn()
    {
        StackBesideSingle s;
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.height(s.top), FullHeight);
        QVERIFY(s.fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(s.height(s.top), s.height(s.single));
        VERIFY_INVARIANTS(s.fixture);
    }

    void presetMissingOnTheNewMonitorKeepsThePixelHeight()
    {
        Config::Config config = instantConfig();
        config.layout.presetWindowHeights = {Config::PresetSize(Config::Fixed {250}), Config::PresetSize(Config::Fixed {400})};
        Config::OutputConfig small;
        small.name = QStringLiteral("DP-2");
        small.layout = Config::LayoutPart {};
        small.layout->presetWindowHeights = QList<Config::PresetSize> {Config::PresetSize(Config::Fixed {600})};
        config.outputs.append(small);
        StackBesideSingle s(config);
        s.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        s.fixture.engine().activateWindow(s.top);
        s.fixture.settle();
        QVERIFY(s.fixture.perform(QStringLiteral("switch-preset-window-height")).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("switch-preset-window-height")).ok);
        QCOMPARE(s.height(s.top), 400.0);
        QVERIFY(s.fixture.perform(QStringLiteral("move-window-to-monitor"), {QStringLiteral("DP-2")}).ok);
        QCOMPARE(s.fixture.state(s.top).output, QStringLiteral("DP-2"));
        QCOMPARE(s.height(s.top), 400.0);
        QVERIFY(s.fixture.perform(QStringLiteral("switch-preset-window-height")).ok);
        QCOMPARE(s.height(s.top), 600.0);
        VERIFY_INVARIANTS(s.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutCarriedHeights)
#include "test_layout_carriedheights.moc"
