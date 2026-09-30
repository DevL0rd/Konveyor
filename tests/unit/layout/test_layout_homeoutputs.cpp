#include "helpers.h"

using namespace LayoutTest;

namespace
{

const QString Primary = QStringLiteral("DP-1");
const QString Secondary = QStringLiteral("DP-2");
const QRectF SecondaryGeometry(1920, 0, 1920, 1080);

QStringList namesOn(Fixture &fixture, const QString &output)
{
    QStringList names;
    for (const Layout::WorkspaceState &state : fixture.engine().workspaceStates()) {
        if (state.output == output) {
            names.append(state.name);
        }
    }
    return names;
}

Config::Config namedOnSecondary(bool emptyAbove = false)
{
    Config::Config config = instantConfig();
    config.layout.emptyWorkspaceAboveFirst = emptyAbove;
    config.workspaces = {namedWorkspace(QStringLiteral("chat"), Secondary), namedWorkspace(QStringLiteral("mail"), Secondary)};
    return config;
}

Layout::OutputInfo withSerial(const QString &name, const QString &serial)
{
    Layout::OutputInfo info = makeOutput(name, SecondaryGeometry);
    info.makeModelSerial = serial;
    return info;
}

struct OnSecondary
{
    Fixture fixture;
    Layout::WindowId window = 0;

    explicit OnSecondary(const Config::Config &config = instantConfig())
        : fixture(config)
    {
        fixture.addOutput(Secondary, SecondaryGeometry);
        fixture.engine().focusOutput(Secondary);
        window = fixture.add(QStringLiteral("a"));
    }

    void replug()
    {
        fixture.removeOutput(Secondary);
        fixture.addOutput(Secondary, SecondaryGeometry);
    }
};

}

class TestLayoutHomeOutputs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namedWorkspacesWaitOnTheFirstMonitorForTheirOutput()
    {
        Fixture fixture(namedOnSecondary());
        QCOMPARE(namesOn(fixture, Primary).mid(0, 2), (QStringList {QStringLiteral("chat"), QStringLiteral("mail")}));
        fixture.addOutput(Secondary, SecondaryGeometry);
        QVERIFY(!namesOn(fixture, Primary).contains(QStringLiteral("chat")));
        QCOMPARE(namesOn(fixture, Secondary).mid(0, 2), (QStringList {QStringLiteral("chat"), QStringLiteral("mail")}));
        VERIFY_INVARIANTS(fixture);
    }

    void namedWorkspacesGoHomeInOrderAfterAReplug_data()
    {
        QTest::addColumn<bool>("emptyAbove");
        QTest::newRow("empty above first off") << false;
        QTest::newRow("empty above first on") << true;
    }

    void namedWorkspacesGoHomeInOrderAfterAReplug()
    {
        QFETCH(bool, emptyAbove);
        OnSecondary s(namedOnSecondary(emptyAbove));
        const QStringList before = namesOn(s.fixture, Secondary);
        const QStringList primaryBefore = namesOn(s.fixture, Primary);
        s.fixture.removeOutput(Secondary);
        QVERIFY(namesOn(s.fixture, Primary).contains(QStringLiteral("mail")));
        VERIFY_INVARIANTS(s.fixture);
        s.fixture.addOutput(Secondary, SecondaryGeometry);
        QCOMPARE(namesOn(s.fixture, Secondary), before);
        QCOMPARE(namesOn(s.fixture, Primary), primaryBefore);
        QCOMPARE(s.fixture.state(s.window).output, Secondary);
        VERIFY_INVARIANTS(s.fixture);
    }

    void replugRestoresTheActiveWorkspace()
    {
        OnSecondary s;
        QVERIFY(s.fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        const auto lower = s.fixture.add(QStringLiteral("b"));
        const Layout::WorkspaceId active = s.fixture.state(lower).workspace;
        s.replug();
        QCOMPARE(s.fixture.state(lower).output, Secondary);
        QVERIFY(s.fixture.state(lower).onActiveWorkspace);
        QVERIFY(!s.fixture.state(s.window).onActiveWorkspace);
        QCOMPARE(s.fixture.state(lower).workspace, active);
        VERIFY_INVARIANTS(s.fixture);
    }

    void outputComingBackUnderANewNameIsMatchedBySerial()
    {
        Fixture fixture;
        fixture.engine().addOutput(withSerial(Secondary, QStringLiteral("Dell U2720Q 1234")));
        fixture.engine().focusOutput(Secondary);
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.removeOutput(Secondary);
        QCOMPARE(fixture.state(id).output, Primary);
        fixture.engine().addOutput(withSerial(QStringLiteral("DP-5"), QStringLiteral("Dell U2720Q 1234")));
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-5"));
        VERIFY_INVARIANTS(fixture);
    }

    void differentMonitorOnTheSameConnectorDoesNotTakeTheWorkspaces()
    {
        Fixture fixture;
        fixture.engine().addOutput(withSerial(Secondary, QStringLiteral("Dell U2720Q 1234")));
        fixture.engine().focusOutput(Secondary);
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.removeOutput(Secondary);
        fixture.engine().addOutput(withSerial(Secondary, QStringLiteral("LG 27GL850 9999")));
        fixture.settle();
        QCOMPARE(fixture.state(id).output, Primary);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceGivenANewWindowWhileAwayStaysOnItsNewMonitor()
    {
        OnSecondary s;
        s.fixture.removeOutput(Secondary);
        QVERIFY(s.fixture.perform(QStringLiteral("focus-window"), {}, idProperty(s.window)).ok);
        const auto added = s.fixture.add(QStringLiteral("b"));
        QCOMPARE(s.fixture.state(added).workspace, s.fixture.state(s.window).workspace);
        s.fixture.addOutput(Secondary, SecondaryGeometry);
        QCOMPARE(s.fixture.state(s.window).output, Primary);
        QCOMPARE(s.fixture.state(added).output, Primary);
        VERIFY_INVARIANTS(s.fixture);
    }

    void emptyWorkspacesAreNotCarriedAround()
    {
        OnSecondary s;
        const int before = static_cast<int>(s.fixture.engine().workspaceStates().size());
        s.replug();
        QCOMPARE(static_cast<int>(s.fixture.engine().workspaceStates().size()), before);
        s.fixture.remove(s.window);
        s.replug();
        QCOMPARE(namesOn(s.fixture, Primary).size(), 1);
        QCOMPARE(namesOn(s.fixture, Secondary).size(), 1);
        VERIFY_INVARIANTS(s.fixture);
    }

    void floatingWindowsTravelWithTheirWorkspace()
    {
        OnSecondary s;
        QVERIFY(s.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        s.fixture.removeOutput(Secondary);
        QVERIFY(s.fixture.state(s.window).isFloating);
        QCOMPARE(s.fixture.state(s.window).output, Primary);
        s.fixture.addOutput(Secondary, SecondaryGeometry);
        QVERIFY(s.fixture.state(s.window).isFloating);
        QCOMPARE(s.fixture.state(s.window).output, Secondary);
        VERIFY_INVARIANTS(s.fixture);
    }

    void workspaceMovedBackToItsFirstMonitorBelongsThereAgain()
    {
        OnSecondary s;
        QVERIFY(s.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {Primary}).ok);
        QCOMPARE(s.fixture.state(s.window).output, Primary);
        QVERIFY(s.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {Secondary}).ok);
        QCOMPARE(s.fixture.state(s.window).output, Secondary);
        s.replug();
        QCOMPARE(s.fixture.state(s.window).output, Secondary);
        VERIFY_INVARIANTS(s.fixture);
    }

    void namedWorkspaceMovedToAnotherMonitorGoesHomeThereAfterAReplug()
    {
        OnSecondary s(namedOnSecondary());
        QVERIFY(s.fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("mail")}).ok);
        QVERIFY(s.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {Primary}).ok);
        QVERIFY(namesOn(s.fixture, Primary).contains(QStringLiteral("mail")));
        s.fixture.removeOutput(Primary);
        s.fixture.addOutput(Primary, QRectF(0, 0, 1920, 1080));
        QVERIFY(namesOn(s.fixture, Primary).contains(QStringLiteral("mail")));
        QVERIFY(!namesOn(s.fixture, Secondary).contains(QStringLiteral("mail")));
        VERIFY_INVARIANTS(s.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutHomeOutputs)
#include "test_layout_homeoutputs.moc"
