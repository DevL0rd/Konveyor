#include "launcherharness.h"

#include <QQmlIncubationController>

class TestLauncherPageSwitchingQml : public LauncherTest::TestCase
{
    Q_OBJECT

    static constexpr int Switches = 200;
    static constexpr int HostRebuilds = 40;

    int m_interrupted = 0;

    QQmlIncubationController *controller() { return m_harness.engine().incubationController(); }

    void letLoadingProgress(int step)
    {
        if (controller() && controller()->incubatingObjectCount() > 0) {
            ++m_interrupted;
        }
        if (step % 3 == 1) {
            QTest::qWait(0);
        } else if (step % 3 == 2 && controller()) {
            controller()->incubateFor(step % 7);
            QTest::qWait(0);
        }
    }

    void showSettingsPage(const QString &id)
    {
        eval(QStringLiteral("launcherData.settingsTarget = { page: '%1', section: '', label: '' }").arg(id));
    }

    static int svgItems(const QObject *root)
    {
        const QList<QObject *> all = root->findChildren<QObject *>();
        return std::count_if(all.cbegin(), all.cend(),
            [](const QObject *object) { return object->inherits("KSvg::SvgItem") || object->inherits("KSvg::FrameSvgItem"); });
    }

    QObject *hostLoadingSearch(int step)
    {
        QObject *host = m_harness.host(false);
        if (!host) {
            return nullptr;
        }
        host->setProperty("open", true);
        if (!QTest::qWaitFor([this] { return controller()->incubatingObjectCount() > 0; }, 30000)) {
            return nullptr;
        }
        controller()->incubateFor(step);
        return host;
    }

private Q_SLOTS:
    void settingsPagesCreateNoSvgItems()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("settings")));
        const QObject *view = eval(QStringLiteral("launcher.currentView()")).value<QObject *>();
        QVERIFY(view);
        const QVariantList entries = eval(QStringLiteral("launcher.currentView().sections[0].entries")).toList();
        QVERIFY(!entries.isEmpty());
        for (const QVariant &entry : entries) {
            const QString id = entry.toMap().value(QStringLiteral("id")).toString();
            showSettingsPage(id);
            QVERIFY(m_harness.finishIncubating());
            QVERIFY2(svgItems(view) == 0, qPrintable(id));
        }
    }

    void leavingSettingsWhileItLoads()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(m_harness.finishIncubating());
        showSettingsPage(QStringLiteral("appearance"));
        m_interrupted = 0;
        for (int step = 0; step < Switches; ++step) {
            QVERIFY(goTo(QStringLiteral("settings")));
            letLoadingProgress(step);
            QVERIFY(goTo(QStringLiteral("home")));
            letLoadingProgress(step + 1);
        }
        QVERIFY(m_interrupted > 0);
    }

    void changingSettingsPagesWhileTheyLoad()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("settings")));
        const QStringList pages {QStringLiteral("appearance"), QStringLiteral("taskbar")};
        m_interrupted = 0;
        for (int step = 0; step < Switches; ++step) {
            showSettingsPage(pages.at(step % 2));
            letLoadingProgress(step);
        }
        QVERIFY(m_interrupted > 0);
    }

    void closingWhileSearchLoadsKeepsLoadingIt()
    {
        for (int step = 0; step < HostRebuilds; ++step) {
            QVERIFY(hostLoadingSearch(step));
            eval(QStringLiteral("launcher.shown = false"));
            QTest::qWait(0);
            QVERIFY(m_harness.finishIncubating());
            QVERIFY(eval(QStringLiteral("searchLoader.item !== null")).toBool());
            m_harness.reset();
        }
    }

    void destroyingTheHostWhileSearchLoads()
    {
        for (int step = 0; step < HostRebuilds; ++step) {
            QVERIFY(hostLoadingSearch(step));
            m_harness.reset();
        }
    }
};

LAUNCHER_TEST_MAIN(TestLauncherPageSwitchingQml)
#include "test_launcher_page_switching_qml.moc"
