#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Main = QStringLiteral("DP-1");
const QString Extra = QStringLiteral("DP-2");
const QRectF ExtraGeometry(1920, 0, 1920, 1080);

Config::NamedWorkspace named(const QString &name, const QString &output)
{
    Config::NamedWorkspace workspace;
    workspace.name = name;
    workspace.openOnOutput = output;
    return workspace;
}

Config::Config namedConfig(bool emptyAbove)
{
    Config::Config config = instantConfig();
    config.layout.emptyWorkspaceAboveFirst = emptyAbove;
    config.workspaces = {named(QStringLiteral("mail"), Main), named(QStringLiteral("web"), Extra), named(QStringLiteral("chat"), Main)};
    return config;
}

QStringList withEdges(bool emptyAbove, const QStringList &names)
{
    QStringList result = emptyAbove ? QStringList {QString()} : QStringList {};
    result.append(names);
    result.append(QString());
    return result;
}

struct NamedOnTwoOutputs
{
    bool emptyAbove;
    Fixture fixture;
    Layout::WindowId onWeb = 0;
    Layout::WindowId onChat = 0;

    explicit NamedOnTwoOutputs(bool above)
        : emptyAbove(above)
        , fixture(namedConfig(above))
    {
        addOutputAt(fixture, Extra, ExtraGeometry);
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("web")});
        onWeb = fixture.add(QStringLiteral("browser"));
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("chat")});
        onChat = fixture.add(QStringLiteral("chat"));
    }

    void unplugExtra()
    {
        fixture.engine().removeOutput(Extra);
        fixture.advance(1);
    }

    void replugExtra()
    {
        addOutputAt(fixture, Extra, ExtraGeometry);
        fixture.advance(1);
    }

    QString nameOfWindow(Layout::WindowId id)
    {
        const auto [output, index] = placeOf(fixture, id);
        return namesOn(fixture, output).value(index - 1);
    }
};

void verifyPlugged(NamedOnTwoOutputs &f)
{
    QCOMPARE(namesOn(f.fixture, Main), withEdges(f.emptyAbove, {QStringLiteral("mail"), QStringLiteral("chat")}));
    QCOMPARE(namesOn(f.fixture, Extra), withEdges(f.emptyAbove, {QStringLiteral("web")}));
    QCOMPARE(f.nameOfWindow(f.onWeb), QStringLiteral("web"));
    QCOMPARE(f.nameOfWindow(f.onChat), QStringLiteral("chat"));
    QCOMPARE(f.fixture.state(f.onWeb).output, Extra);
    VERIFY_INVARIANTS(f.fixture);
}

void verifyUnplugged(NamedOnTwoOutputs &f)
{
    QCOMPARE(namesOn(f.fixture, Main), withEdges(f.emptyAbove, {QStringLiteral("mail"), QStringLiteral("chat"), QStringLiteral("web")}));
    QCOMPARE(f.nameOfWindow(f.onWeb), QStringLiteral("web"));
    QCOMPARE(f.fixture.state(f.onWeb).output, Main);
    VERIFY_INVARIANTS(f.fixture);
}

}

class TestLayoutNamedOutputs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namedWorkspacesFollowTheirOutputThroughUnplugAndReplug_data()
    {
        QTest::addColumn<bool>("emptyAbove");
        QTest::newRow("no empty workspace above") << false;
        QTest::newRow("empty workspace above first") << true;
    }

    void namedWorkspacesFollowTheirOutputThroughUnplugAndReplug()
    {
        QFETCH(bool, emptyAbove);
        NamedOnTwoOutputs f(emptyAbove);
        verifyPlugged(f);
        f.unplugExtra();
        verifyUnplugged(f);
        COMPARE_FOCUS(f.fixture, f.onChat);
        f.replugExtra();
        verifyPlugged(f);
        COMPARE_FOCUS(f.fixture, f.onChat);
    }

    void namedWorkspaceOfAMissingOutputWaitsOnAnotherOutput_data() { namedWorkspacesFollowTheirOutputThroughUnplugAndReplug_data(); }

    void namedWorkspaceOfAMissingOutputWaitsOnAnotherOutput()
    {
        QFETCH(bool, emptyAbove);
        Fixture fixture(namedConfig(emptyAbove));
        QCOMPARE(namesOn(fixture, Main), withEdges(emptyAbove, {QStringLiteral("mail"), QStringLiteral("web"), QStringLiteral("chat")}));
        addOutputAt(fixture, Extra, ExtraGeometry);
        QCOMPARE(namesOn(fixture, Main), withEdges(emptyAbove, {QStringLiteral("mail"), QStringLiteral("chat")}));
        QCOMPARE(namesOn(fixture, Extra), withEdges(emptyAbove, {QStringLiteral("web")}));
        VERIFY_INVARIANTS(fixture);
    }

    void focusedNamedWorkspaceTakenByItsOutputLeavesAValidActiveWorkspace_data()
    {
        namedWorkspacesFollowTheirOutputThroughUnplugAndReplug_data();
    }

    void focusedNamedWorkspaceTakenByItsOutputLeavesAValidActiveWorkspace()
    {
        QFETCH(bool, emptyAbove);
        NamedOnTwoOutputs f(emptyAbove);
        f.unplugExtra();
        act(f.fixture, QStringLiteral("focus-workspace"), {QStringLiteral("web")});
        COMPARE_FOCUS(f.fixture, f.onWeb);
        f.replugExtra();
        verifyPlugged(f);
        const int active = activeWorkspaceOn(f.fixture, Main);
        QVERIFY(active >= 1 && active <= static_cast<int>(workspacesOn(f.fixture, Main).size()));
        QCOMPARE(activeWorkspaceOn(f.fixture, Extra), emptyAbove ? 2 : 1);
    }

    void firstNamedWorkspaceTakenAwayKeepsTheEmptyWorkspaceAbove_data() { namedWorkspacesFollowTheirOutputThroughUnplugAndReplug_data(); }

    void firstNamedWorkspaceTakenAwayKeepsTheEmptyWorkspaceAbove()
    {
        QFETCH(bool, emptyAbove);
        Config::Config config = namedConfig(emptyAbove);
        config.workspaces = {named(QStringLiteral("web"), Extra), named(QStringLiteral("mail"), Main)};
        Fixture fixture(config);
        act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("mail")});
        const auto mail = fixture.add();
        addOutputAt(fixture, Extra, ExtraGeometry);
        QCOMPARE(namesOn(fixture, Main), withEdges(emptyAbove, {QStringLiteral("mail")}));
        QCOMPARE(namesOn(fixture, Extra), withEdges(emptyAbove, {QStringLiteral("web")}));
        QCOMPARE(placeOf(fixture, mail), std::pair(Main, emptyAbove ? 2 : 1));
        QCOMPARE(activeWorkspaceOn(fixture, Main), emptyAbove ? 2 : 1);
        COMPARE_FOCUS(fixture, mail);
        VERIFY_INVARIANTS(fixture);
    }

    void namedWorkspaceGivenAnOutputLaterJoinsTheNamedWorkspacesOfThatOutput_data()
    {
        namedWorkspacesFollowTheirOutputThroughUnplugAndReplug_data();
    }

    void namedWorkspaceGivenAnOutputLaterJoinsTheNamedWorkspacesOfThatOutput()
    {
        QFETCH(bool, emptyAbove);
        Config::Config config = namedConfig(emptyAbove);
        config.workspaces = {named(QStringLiteral("web"), Extra)};
        Fixture fixture(config);
        addOutputAt(fixture, Extra, ExtraGeometry);
        addOn(fixture, Extra);
        act(fixture, QStringLiteral("focus-workspace-down"));
        addOn(fixture, Extra);
        fixture.engine().focusOutput(Main);
        Config::NamedWorkspace chat;
        chat.name = QStringLiteral("chat");
        config.workspaces.append(chat);
        fixture.setConfig(config);
        fixture.advance(1);
        QCOMPARE(namesOn(fixture, Main), withEdges(emptyAbove, {QStringLiteral("chat")}));
        QCOMPARE(namesOn(fixture, Extra), withEdges(emptyAbove, {QStringLiteral("web"), QString()}));
        config.workspaces.last().openOnOutput = Extra;
        fixture.setConfig(config);
        fixture.advance(1);
        QCOMPARE(namesOn(fixture, Extra), withEdges(emptyAbove, {QStringLiteral("web"), QStringLiteral("chat"), QString()}));
        VERIFY_INVARIANTS(fixture);
    }

    void togglingEmptyWorkspaceAboveFirstLiveAddsAndRemovesIt()
    {
        NamedOnTwoOutputs f(false);
        Config::Config config = namedConfig(true);
        f.fixture.setConfig(config);
        f.fixture.advance(1);
        f.emptyAbove = true;
        verifyPlugged(f);
        COMPARE_FOCUS(f.fixture, f.onChat);
        config.layout.emptyWorkspaceAboveFirst = false;
        f.fixture.setConfig(config);
        f.fixture.advance(1);
        f.emptyAbove = false;
        verifyPlugged(f);
        COMPARE_FOCUS(f.fixture, f.onChat);
    }
};

QTEST_GUILESS_MAIN(TestLayoutNamedOutputs)
#include "test_layout_namedoutputs.moc"
