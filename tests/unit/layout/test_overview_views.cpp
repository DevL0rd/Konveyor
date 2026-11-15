#include "overviewhelpers.h"

#include <QTest>

using namespace LayoutTest;

namespace
{

bool containsWindow(const Layout::OverviewView &view, Layout::WindowId id)
{
    return std::ranges::any_of(view.windows, [id](const Layout::OverviewWindow &window) { return window.id == id; });
}

}

class TestOverviewViews : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void closedOverviewHasNoViews();
    void oneOutputPaintsEveryWindow();
    void twoOutputsGetSeparateViews();
    void blankWorkspaceStillGetsACard();
    void selectionFollowsFocus();
    void panOnlyMovesItsOwnOutput();
    void reservedInsetsOnlyShrinkTheirOutput();
    void closingClearsViews();
};

void TestOverviewViews::closedOverviewHasNoViews()
{
    Fixture fixture(overviewConfig());
    fixture.add(QStringLiteral("a"));
    QVERIFY(fixture.engine().isOverviewEnabled());
    QVERIFY(fixture.engine().overviewViews().isEmpty());
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
}

void TestOverviewViews::oneOutputPaintsEveryWindow()
{
    Fixture fixture(overviewConfig());
    const auto a = fixture.add(QStringLiteral("a"));
    const auto b = fixture.add(QStringLiteral("b"));

    fixture.engine().setOverviewOpen(true);
    QCOMPARE(fixture.engine().overviewProgress(), 1.0);

    const QList<Layout::OverviewView> views = fixture.engine().overviewViews();
    QCOMPARE(views.size(), 1);
    QCOMPARE(views[0].output, QStringLiteral("DP-1"));
    QCOMPARE(views[0].outputGeometry, QRectF(0, 0, 1920, 1080));
    // Dynamic trailing empty workspaces are hidden while idle.
    QCOMPARE(views[0].workspaceRects.size(), 1);
    QCOMPARE(views[0].windows.size(), 2);
    QVERIFY(containsWindow(views[0], a));
    QVERIFY(containsWindow(views[0], b));
    QVERIFY(views[0].scale > 0.4);
    QVERIFY(views[0].camera.y() <= views[0].workspaceRects[0].top());
}

void TestOverviewViews::twoOutputsGetSeparateViews()
{
    Fixture fixture(overviewConfig());
    fixture.addOutput(QStringLiteral("HDMI-A-1"), QRectF(1920, 0, 1280, 720));
    const auto laptop = fixture.add(QStringLiteral("laptop"));
    const auto external
        = fixture.addClient(makeWindow(QStringLiteral("external")), Client(), Layout::ActivationPolicy::Focus, QStringLiteral("HDMI-A-1"));

    fixture.engine().setOverviewOpen(true);
    const QList<Layout::OverviewView> views = fixture.engine().overviewViews();
    QCOMPARE(views.size(), 2);

    const Layout::OverviewView *laptopView = viewFor(views, QStringLiteral("DP-1"));
    const Layout::OverviewView *externalView = viewFor(views, QStringLiteral("HDMI-A-1"));
    QVERIFY(laptopView);
    QVERIFY(externalView);
    QVERIFY(containsWindow(*laptopView, laptop));
    QVERIFY(!containsWindow(*laptopView, external));
    QVERIFY(containsWindow(*externalView, external));
    QVERIFY(!containsWindow(*externalView, laptop));
    QCOMPARE(externalView->outputGeometry, QRectF(1920, 0, 1280, 720));

    for (const Layout::OverviewWindow &window : externalView->windows) {
        QVERIFY(window.rect.left() >= 1920 - 0.5);
        QVERIFY(window.rect.right() <= 1920 + 1280 + 0.5);
    }
    for (const Layout::OverviewWindow &window : laptopView->windows) {
        QVERIFY(window.rect.left() <= 1920);
    }
}

void TestOverviewViews::blankWorkspaceStillGetsACard()
{
    Config::Config config = overviewConfig();
    config.workspaces = {namedWorkspace(QStringLiteral("web")), namedWorkspace(QStringLiteral("chat"))};
    Fixture fixture(config);
    const auto a = fixture.add(QStringLiteral("a"));

    fixture.engine().setOverviewOpen(true);
    const QList<Layout::OverviewView> views = fixture.engine().overviewViews();
    QCOMPARE(views.size(), 1);
    QCOMPARE(views[0].workspaceRects.size(), 2);
    QCOMPARE(views[0].windows.size(), 1);
    QCOMPARE(views[0].windows[0].id, a);
    QCOMPARE(views[0].windows[0].workspaceIndex, 1);
    QCOMPARE(views[0].activeWorkspace, 0);
    QVERIFY(views[0].workspaceRects[0] != views[0].workspaceRects[1]);
    // The named blank card is shown, while the trailing unnamed empty workspace is hidden.
    QVERIFY(std::ranges::none_of(views[0].windows, [](const Layout::OverviewWindow &window) { return window.workspaceIndex == 2; }));
}

void TestOverviewViews::selectionFollowsFocus()
{
    FocusedPair pair;
    QCOMPARE(pair.fixture.focused(), std::optional(pair.b));

    pair.fixture.engine().setOverviewOpen(true);
    QCOMPARE(pair.fixture.engine().overviewSelectedWindow(), std::optional(pair.b));
    for (const Layout::OverviewWindow &window : pair.fixture.engine().overviewViews()[0].windows) {
        QCOMPARE(window.selected, window.id == pair.b);
        QCOMPARE(window.focused, window.id == pair.b);
    }

    pair.fixture.engine().overviewSelectWindow(pair.a);
    QCOMPARE(pair.fixture.engine().overviewSelectedWindow(), std::optional(pair.a));

    pair.fixture.engine().overviewSelectWindow(9999);
    QCOMPARE(pair.fixture.engine().overviewSelectedWindow(), std::optional(pair.a));
}

void TestOverviewViews::panOnlyMovesItsOwnOutput()
{
    Config::Config config = overviewConfig();
    config.workspaces
        = {namedWorkspace(QStringLiteral("web")), namedWorkspace(QStringLiteral("chat")), namedWorkspace(QStringLiteral("mail"))};
    TwoOutputs outputs(config);
    Fixture &fixture = outputs.fixture;
    const QList<Layout::OverviewView> before = outputs.openOneOnEach();
    const Layout::OverviewView *laptopBefore = viewFor(before, QStringLiteral("DP-1"));
    const QPointF externalCamera = viewFor(before, QStringLiteral("HDMI-A-1"))->camera;
    QVERIFY(laptopBefore->workspaceRects.size() >= 3);

    fixture.engine().overviewPan(QStringLiteral("DP-1"), QPointF(0, 25));
    const QList<Layout::OverviewView> after = fixture.engine().overviewViews();
    const Layout::OverviewView *laptopAfter = viewFor(after, QStringLiteral("DP-1"));
    QCOMPARE(laptopAfter->camera, laptopBefore->camera + QPointF(0, 25));
    QCOMPARE(viewFor(after, QStringLiteral("HDMI-A-1"))->camera, externalCamera);

    fixture.engine().overviewPan(QStringLiteral("DP-1"), QPointF(0, 1e6));
    const Layout::OverviewView *clamped = viewFor(fixture.engine().overviewViews(), QStringLiteral("DP-1"));
    QCOMPARE(clamped->camera.y(), clamped->contentBounds.bottom() - 1080);
    QCOMPARE(clamped->camera.x(), laptopAfter->camera.x());
}

void TestOverviewViews::reservedInsetsOnlyShrinkTheirOutput()
{
    TwoOutputs outputs;
    Fixture &fixture = outputs.fixture;
    const QList<Layout::OverviewView> before = outputs.openOneOnEach();
    const double laptopScale = viewFor(before, QStringLiteral("DP-1"))->scale;
    const double externalScale = viewFor(before, QStringLiteral("HDMI-A-1"))->scale;

    fixture.engine().setOverviewReserved(QStringLiteral("DP-1"), QMarginsF(0, 0, 0, 400));
    const QList<Layout::OverviewView> after = fixture.engine().overviewViews();
    const Layout::OverviewView *laptop = viewFor(after, QStringLiteral("DP-1"));
    const Layout::OverviewView *external = viewFor(after, QStringLiteral("HDMI-A-1"));
    QCOMPARE(laptop->reserved, QMarginsF(0, 0, 0, 400));
    QVERIFY(laptop->scale < laptopScale);
    QCOMPARE(external->reserved, QMarginsF());
    QCOMPARE(external->scale, externalScale);
}

void TestOverviewViews::closingClearsViews()
{
    Fixture fixture(overviewConfig());
    fixture.add(QStringLiteral("a"));
    fixture.engine().setOverviewOpen(true);
    QCOMPARE(fixture.engine().overviewViews().size(), 1);

    fixture.engine().setOverviewOpen(false);
    QCOMPARE(fixture.engine().overviewProgress(), 0.0);
    QVERIFY(fixture.engine().overviewViews().isEmpty());
    QVERIFY(!fixture.engine().overviewSelectedWindow());
}

QTEST_MAIN(TestOverviewViews)
#include "test_overview_views.moc"
