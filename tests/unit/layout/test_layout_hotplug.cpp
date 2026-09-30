#include "actionhelpers.h"

#include <QHash>
#include <QRandomGenerator>

using namespace LayoutTest;

namespace
{

const QString Plain = QStringLiteral("DP-1");

QList<Layout::OutputInfo> monitors()
{
    return {
        makeOutput(QStringLiteral("HDMI-A-1"), QRectF(-1280, 0, 1280, 1024), 1.0, QStringLiteral("Dell P1917S 7")),
        makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 2560, 1440), 1.5, QStringLiteral("Acme 27Q")),
        makeOutput(QStringLiteral("DP-3"), QRectF(4480, 0, 2560, 1440), 1.5, QStringLiteral("Acme 27Q")),
    };
}

struct Plugged
{
    Fixture fixture;
    QHash<Layout::WindowId, QString> homes;
    QHash<Layout::WindowId, Layout::WindowId> sharesWorkspaceWith;

    explicit Plugged(const Config::Config &config = instantConfig())
        : fixture(config)
    {
        for (const Layout::OutputInfo &output : monitors()) {
            fixture.addOutput(output);
        }
        for (const QString &output : {Plain, QStringLiteral("HDMI-A-1"), QStringLiteral("DP-2"), QStringLiteral("DP-3")}) {
            const auto first = addOn(fixture, output, output + QStringLiteral("-a"));
            const auto second = fixture.add(output + QStringLiteral("-b"));
            QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
            const auto lower = fixture.add(output + QStringLiteral("-c"));
            homes.insert(first, output);
            homes.insert(second, output);
            homes.insert(lower, output);
            sharesWorkspaceWith.insert(second, first);
        }
    }

    Layout::OutputInfo info(const QString &name) const
    {
        for (const Layout::OutputInfo &output : monitors()) {
            if (output.name == name) {
                return output;
            }
        }
        return makeOutput(Plain, QRectF(0, 0, 1920, 1080));
    }

    QString everyWindowAtHome()
    {
        for (auto it = homes.cbegin(); it != homes.cend(); ++it) {
            if (fixture.state(it.key()).output != it.value()) {
                return QStringLiteral("window %1 is on %2, not %3").arg(it.key()).arg(fixture.state(it.key()).output, it.value());
            }
        }
        for (auto it = sharesWorkspaceWith.cbegin(); it != sharesWorkspaceWith.cend(); ++it) {
            if (fixture.state(it.key()).workspace != fixture.state(it.value()).workspace) {
                return QStringLiteral("windows %1 and %2 were split up").arg(it.key()).arg(it.value());
            }
        }
        return {};
    }

    QString everyWindowPresent()
    {
        for (auto it = homes.cbegin(); it != homes.cend(); ++it) {
            if (!fixture.engine().hasWindow(it.key())) {
                return QStringLiteral("window %1 is gone").arg(it.key());
            }
        }
        return fixture.invariants();
    }
};

QStringList allNames()
{
    return {Plain, QStringLiteral("HDMI-A-1"), QStringLiteral("DP-2"), QStringLiteral("DP-3")};
}

}

class TestLayoutHotplug : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everyWindowComesHomeAfterRandomHotplugs_data()
    {
        QTest::addColumn<quint32>("seed");
        for (quint32 seed = 1; seed <= 40; ++seed) {
            QTest::newRow(qPrintable(QStringLiteral("seed %1").arg(seed))) << seed;
        }
    }

    void everyWindowComesHomeAfterRandomHotplugs()
    {
        QFETCH(quint32, seed);
        QRandomGenerator random(seed);
        Plugged plugged;
        QVERIFY2(plugged.everyWindowAtHome().isEmpty(), qPrintable(plugged.everyWindowAtHome()));
        QStringList connected = allNames();
        for (int step = 0; step < 30; ++step) {
            const QString name = allNames().at(random.bounded(static_cast<int>(allNames().size())));
            if (connected.contains(name)) {
                plugged.fixture.removeOutput(name);
                connected.removeOne(name);
            } else {
                plugged.fixture.addOutput(plugged.info(name));
                connected.append(name);
            }
            if (random.bounded(4) == 0 && !connected.isEmpty()) {
                Layout::OutputInfo moved = plugged.info(connected.first());
                moved.scale = random.bounded(2) == 0 ? 1.25 : 2.0;
                plugged.fixture.engine().updateOutput(moved);
                plugged.fixture.settle();
            }
            QVERIFY2(plugged.everyWindowPresent().isEmpty(), qPrintable(plugged.everyWindowPresent()));
        }
        QStringList missing = allNames();
        for (const QString &name : connected) {
            missing.removeOne(name);
        }
        while (!missing.isEmpty()) {
            plugged.fixture.addOutput(plugged.info(missing.takeAt(random.bounded(static_cast<int>(missing.size())))));
        }
        QVERIFY2(plugged.everyWindowAtHome().isEmpty(), qPrintable(plugged.everyWindowAtHome()));
        VERIFY_INVARIANTS(plugged.fixture);
    }

    void movedWorkspacesStillComeHome_data() { everyWindowComesHomeAfterRandomHotplugs_data(); }

    void movedWorkspacesStillComeHome()
    {
        QFETCH(quint32, seed);
        QRandomGenerator random(seed);
        Plugged plugged;
        for (int move = 0; move < 6; ++move) {
            const QList<Layout::WindowId> windows = plugged.homes.keys();
            const Layout::WindowId window = windows.at(random.bounded(static_cast<int>(windows.size())));
            const QString target = allNames().at(random.bounded(static_cast<int>(allNames().size())));
            QVERIFY(plugged.fixture.perform(QStringLiteral("focus-window"), {}, idProperty(window)).ok);
            QVERIFY(plugged.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {target}).ok);
            VERIFY_INVARIANTS(plugged.fixture);
        }
        QStringList order = allNames();
        for (const QString &name : order) {
            plugged.fixture.removeOutput(name);
        }
        while (!order.isEmpty()) {
            plugged.fixture.addOutput(plugged.info(order.takeAt(random.bounded(static_cast<int>(order.size())))));
        }
        QVERIFY2(plugged.everyWindowAtHome().isEmpty(), qPrintable(plugged.everyWindowAtHome()));
        VERIFY_INVARIANTS(plugged.fixture);
    }

    void monitorsSwappingConnectorsKeepTheirWorkspaces()
    {
        Fixture fixture;
        fixture.removeOutput(Plain);
        fixture.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080), 1.0, QStringLiteral("Dell 1")));
        fixture.addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080), 1.0, QStringLiteral("LG 2")));
        const auto onDell = addOn(fixture, QStringLiteral("DP-1"), QStringLiteral("dell"));
        const auto onLg = addOn(fixture, QStringLiteral("DP-2"), QStringLiteral("lg"));
        fixture.removeOutput(QStringLiteral("DP-2"));
        fixture.removeOutput(QStringLiteral("DP-1"));
        fixture.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080), 1.0, QStringLiteral("LG 2")));
        fixture.addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080), 1.0, QStringLiteral("Dell 1")));
        QCOMPARE(fixture.state(onDell).output, QStringLiteral("DP-2"));
        QCOMPARE(fixture.state(onLg).output, QStringLiteral("DP-1"));
        VERIFY_INVARIANTS(fixture);
    }

    void unplugDuringAnAnimatedWorkspaceSwitch()
    {
        Plugged plugged(linearAnimationConfig());
        plugged.fixture.advanceInSteps(500);
        plugged.fixture.engine().focusOutput(QStringLiteral("DP-2"));
        QVERIFY(plugged.fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        plugged.fixture.advance(50);
        plugged.fixture.removeOutput(QStringLiteral("DP-2"));
        plugged.fixture.advanceInSteps(500);
        QVERIFY2(plugged.everyWindowPresent().isEmpty(), qPrintable(plugged.everyWindowPresent()));
        plugged.fixture.addOutput(plugged.info(QStringLiteral("DP-2")));
        QVERIFY(plugged.fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        plugged.fixture.advance(50);
        plugged.fixture.removeOutput(QStringLiteral("DP-2"));
        plugged.fixture.addOutput(plugged.info(QStringLiteral("DP-2")));
        plugged.fixture.advanceInSteps(500);
        QVERIFY2(plugged.everyWindowAtHome().isEmpty(), qPrintable(plugged.everyWindowAtHome()));
        VERIFY_INVARIANTS(plugged.fixture);
    }

    void focusFollowsTheWindowWhenItsMonitorGoes()
    {
        Plugged plugged;
        plugged.fixture.engine().focusOutput(QStringLiteral("DP-3"));
        const auto focused = plugged.fixture.focused();
        QVERIFY(focused);
        plugged.fixture.removeOutput(QStringLiteral("DP-3"));
        QVERIFY(plugged.fixture.focused());
        QVERIFY(plugged.fixture.engine().windowState(*focused));
        plugged.fixture.addOutput(plugged.info(QStringLiteral("DP-3")));
        QCOMPARE(plugged.fixture.state(*focused).output, QStringLiteral("DP-3"));
        VERIFY_INVARIANTS(plugged.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutHotplug)
#include "test_layout_hotplug.moc"
