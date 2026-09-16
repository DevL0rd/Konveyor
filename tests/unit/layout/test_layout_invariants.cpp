#include "helpers.h"

#include <QRandomGenerator>

#include <limits>

using namespace LayoutTest;

namespace
{

const QStringList &randomActions()
{
    static const QStringList names {QStringLiteral("focus-column-left"), QStringLiteral("focus-column-right"),
        QStringLiteral("focus-window-down"), QStringLiteral("focus-window-up"), QStringLiteral("move-column-left"),
        QStringLiteral("move-column-right"), QStringLiteral("move-window-down"), QStringLiteral("move-window-up"),
        QStringLiteral("consume-or-expel-window-left"), QStringLiteral("consume-or-expel-window-right"),
        QStringLiteral("consume-window-into-column"), QStringLiteral("expel-window-from-column"), QStringLiteral("swap-window-left"),
        QStringLiteral("swap-window-right"), QStringLiteral("toggle-column-tabbed-display"), QStringLiteral("center-column"),
        QStringLiteral("center-visible-columns"), QStringLiteral("switch-preset-column-width"),
        QStringLiteral("switch-preset-window-height"), QStringLiteral("reset-window-height"), QStringLiteral("maximize-column"),
        QStringLiteral("maximize-window-to-edges"), QStringLiteral("fullscreen-window"), QStringLiteral("expand-column-to-available-width"),
        QStringLiteral("toggle-window-floating"), QStringLiteral("focus-workspace-down"), QStringLiteral("focus-workspace-up"),
        QStringLiteral("move-window-to-workspace-down"), QStringLiteral("move-window-to-workspace-up"),
        QStringLiteral("move-column-to-workspace-down"), QStringLiteral("move-workspace-down"), QStringLiteral("move-workspace-up"),
        QStringLiteral("focus-monitor-left"), QStringLiteral("focus-monitor-right"), QStringLiteral("move-window-to-monitor-right"),
        QStringLiteral("move-column-to-monitor-left"), QStringLiteral("move-workspace-to-monitor-right")};
    return names;
}

class Harness
{
public:
    explicit Harness(quint32 seed)
        : m_random(seed)
    {
        m_fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
    }

    Fixture &fixture() { return m_fixture; }

    QString step()
    {
        const quint32 choice = m_random.bounded(100);
        if (choice < 25 || m_windows.isEmpty()) {
            addWindow();
        } else if (choice < 35) {
            removeWindow();
        } else if (choice < 40) {
            toggleOutput();
        } else {
            runAction();
        }
        m_fixture.advance(17);
        return m_fixture.invariants();
    }

private:
    void addWindow()
    {
        const QSizeF size(100 + m_random.bounded(800), 100 + m_random.bounded(600));
        Layout::WindowProperties properties = makeWindow(QStringLiteral("app"), QStringLiteral("app"), size);
        if (m_random.bounded(10) == 0 && !m_windows.isEmpty()) {
            properties.parent = m_windows.at(static_cast<int>(m_random.bounded(quint32(m_windows.size()))));
        }
        m_windows.append(m_fixture.addWith(properties));
    }

    void removeWindow()
    {
        const int index = static_cast<int>(m_random.bounded(quint32(m_windows.size())));
        m_fixture.remove(m_windows.takeAt(index));
    }

    void toggleOutput()
    {
        if (m_secondOutput) {
            m_fixture.engine().removeOutput(QStringLiteral("DP-2"));
        } else {
            m_fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        }
        m_secondOutput = !m_secondOutput;
        m_fixture.settle();
    }

    void runAction()
    {
        const QString &name = randomActions().at(static_cast<int>(m_random.bounded(quint32(randomActions().size()))));
        m_fixture.perform(name);
    }

    Fixture m_fixture;
    QRandomGenerator m_random;
    QList<Layout::WindowId> m_windows;
    bool m_secondOutput = true;
};

QString deadSpaceProblem(Fixture &fixture)
{
    struct Span
    {
        double left = std::numeric_limits<double>::max();
        double right = std::numeric_limits<double>::lowest();
    };
    static const QHash<QString, std::pair<double, double>> outputs {
        {QStringLiteral("DP-1"), {0.0, 1920.0}},
        {QStringLiteral("DP-2"), {1920.0, 1280.0}},
    };
    const double gaps = 16.0;
    QHash<QString, Span> spans;
    for (const Layout::WindowState &state : fixture.engine().windowStates()) {
        if (state.isFloating || !state.onActiveWorkspace || !outputs.contains(state.output)) {
            continue;
        }
        Span &span = spans[state.output];
        span.left = std::min(span.left, state.targetFrame.left());
        span.right = std::max(span.right, state.targetFrame.right());
    }
    for (auto it = spans.constBegin(); it != spans.constEnd(); ++it) {
        const auto [origin, width] = outputs.value(it.key());
        const double left = it->left - origin;
        const double right = it->right - origin;
        const bool fitsInView = right - left + gaps * 2.0 <= width + 0.5;
        const bool leftGap = fitsInView ? std::abs(left - gaps) > 0.5 && left > 0.5 : left > gaps + 0.5;
        const bool rightGap = !fitsInView && right < width - gaps - 0.5;
        if (leftGap || rightGap) {
            return QStringLiteral("%1 dead space: row spans %2..%3 on a %4 wide output").arg(it.key()).arg(left).arg(right).arg(width);
        }
    }
    return {};
}

QString runSteps(Harness &harness, int count)
{
    for (int step = 0; step < count; ++step) {
        const QString error = harness.step();
        if (!error.isEmpty()) {
            return QStringLiteral("step %1: %2").arg(step).arg(error);
        }
    }
    return {};
}

QString checkStates(const QList<Layout::WindowState> &states)
{
    QSet<Layout::WindowId> seen;
    for (const Layout::WindowState &state : states) {
        const bool valid = !seen.contains(state.id) && state.workspaceIndex >= 1 && !state.output.isEmpty() && state.renderAlpha >= 0.0
            && state.renderAlpha <= 1.0 && state.targetFrame.width() >= 0.0 && state.targetFrame.height() >= 0.0;
        if (!valid) {
            return QStringLiteral("invalid window state for id %1").arg(state.id);
        }
        seen.insert(state.id);
    }
    return {};
}

QStringList removeAll(Fixture &fixture, const QList<Layout::WindowId> &windows)
{
    QStringList errors;
    for (const Layout::WindowId id : windows) {
        fixture.remove(id);
        fixture.advance(1);
        errors.append(fixture.invariants());
    }
    errors.removeAll(QString());
    return errors;
}

QString cycleSecondOutput(Fixture &fixture)
{
    fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
    fixture.settle();
    QString error = fixture.invariants();
    fixture.perform(QStringLiteral("move-window-to-monitor-right"));
    error = error.isEmpty() ? fixture.invariants() : error;
    fixture.engine().removeOutput(QStringLiteral("DP-2"));
    fixture.settle();
    return error.isEmpty() ? fixture.invariants() : error;
}

}

class TestLayoutInvariants : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void randomizedOperationsNeverLeaveDeadSpace_data()
    {
        QTest::addColumn<quint32>("seed");
        for (quint32 seed = 1; seed <= 12; ++seed) {
            QTest::newRow(qPrintable(QStringLiteral("seed-%1").arg(seed))) << seed;
        }
    }

    void randomizedOperationsNeverLeaveDeadSpace()
    {
        QFETCH(quint32, seed);
        Harness harness(seed);
        for (int step = 0; step < 200; ++step) {
            const QString action = harness.step();
            harness.fixture().settle();
            const QString problem = deadSpaceProblem(harness.fixture());
            QVERIFY2(problem.isEmpty(), qPrintable(QStringLiteral("seed %1 step %2: %3").arg(seed).arg(step).arg(problem)));
        }
    }

    void randomizedOperationsKeepInvariants_data()
    {
        QTest::addColumn<quint32>("seed");
        for (quint32 seed = 1; seed <= 12; ++seed) {
            QTest::newRow(qPrintable(QStringLiteral("seed-%1").arg(seed))) << seed;
        }
    }

    void randomizedOperationsKeepInvariants()
    {
        QFETCH(quint32, seed);
        Harness harness(seed);
        const QString error = runSteps(harness, 200);
        QVERIFY2(error.isEmpty(), qPrintable(QStringLiteral("seed %1: %2").arg(seed).arg(error)));
    }

    void windowStatesRemainConsistent()
    {
        Harness harness(99);
        QStringList errors;
        for (int step = 0; step < 100; ++step) {
            harness.step();
            errors.append(checkStates(harness.fixture().engine().windowStates()));
        }
        errors.removeAll(QString());
        QCOMPARE(errors, QStringList());
    }

    void removingEveryWindowLeavesOneEmptyWorkspacePerOutput()
    {
        Fixture fixture;
        QList<Layout::WindowId> windows;
        for (int i = 0; i < 6; ++i) {
            windows.append(fixture.add(QStringLiteral("app")));
        }
        fixture.perform(QStringLiteral("move-window-to-workspace-down"));
        fixture.advance(1);
        QCOMPARE(removeAll(fixture, windows), QStringList());
        QCOMPARE(fixture.engine().windowStates().size(), 0);
        QVERIFY(fixture.engine().workspaceStates().size() <= 2);
        QVERIFY(fixture.engine().workspaceStates().first().isActive);
        QCOMPARE(fixture.engine().focusedWindow(), std::optional<Layout::WindowId>());
    }

    void outputRemovalAndReadditionKeepInvariants()
    {
        Fixture fixture;
        for (int i = 0; i < 4; ++i) {
            fixture.add(QStringLiteral("app"));
        }
        QStringList errors;
        for (int round = 0; round < 5; ++round) {
            errors.append(cycleSecondOutput(fixture));
        }
        errors.removeAll(QString());
        QCOMPARE(errors, QStringList());
        QCOMPARE(fixture.engine().windowStates().size(), 4);
    }
};

QTEST_GUILESS_MAIN(TestLayoutInvariants)
#include "test_layout_invariants.moc"
