#include "plasmoidharness.h"

#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QTest>
#include <QWindow>

namespace
{

const PlasmoidSpec anyPlasmoid {QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.panel"),
    QStringLiteral("org.devl0rd.sysmon.overlay"), QStringLiteral("cpu"), {}};

const QByteArray compactSource = "import QtQuick\nItem {\n    property bool overlayMode: false\n    property int overlayTargetPid: 0\n"
                                 "    property real overlayBackgroundOpacity: 1\n    readonly property real overlayPreferredWidth: 120\n"
                                 "    signal overlayClicked()\n}\n";
const QByteArray popupSource
    = "import QtQuick\nItem {\n    property bool overlayMode: false\n    readonly property real overlayPreferredWidth: 300\n"
      "    readonly property real overlayPreferredHeight: 200\n    signal overlayCloseRequested()\n}\n";

QString statePath()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/Konveyor-Monitor-Overlay/state.json");
}

QByteArray targets(const QByteArray &list)
{
    return "{\"targets\": " + list + ", \"generation\": 1}";
}

const QByteArray game = R"([{"key": "game", "pid": 200, "windowId": 3, "x": 0, "y": 0, "width": 900, "height": 600, "fullscreen": true},
                            {"key": "shell", "pid": 0, "windowId": 4, "x": 0, "y": 0, "width": 300, "height": 300, "fullscreen": false}])";

QList<QObject *> dialogs(const QString &kind)
{
    QList<QObject *> found;
    const QList<QWindow *> windows = QGuiApplication::allWindows();
    for (QWindow *window : windows) {
        if (window->inherits("PlasmaQuick::Dialog") && window->title().startsWith(QStringLiteral("Konveyor Monitor ") + kind)) {
            found.append(window);
        }
    }
    return found;
}

QObject *loaded(QObject *dialog)
{
    const QList<QObject *> children = dialog->property("mainItem").value<QObject *>()->findChildren<QObject *>();
    for (QObject *child : children) {
        if (child->inherits("QQuickLoader") && child->property("item").value<QObject *>()) {
            return child->property("item").value<QObject *>();
        }
    }
    return nullptr;
}

}

class TestPlasmoidMonitorOverlay : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void init()
    {
        m_harness = std::make_unique<PlasmoidHarness>(anyPlasmoid);
        m_harness->setUp(Form::Planar);
        m_compact = std::make_unique<QQmlComponent>(m_harness->engine());
        m_compact->setData(compactSource, QUrl(QStringLiteral("inline:Compact.qml")));
        m_popup = std::make_unique<QQmlComponent>(m_harness->engine());
        m_popup->setData(popupSource, QUrl(QStringLiteral("inline:Popup.qml")));
        QFile::remove(statePath());
    }

    void cleanup()
    {
        m_harness->unload();
        m_compact.reset();
        m_popup.reset();
        m_harness.reset();
    }

    void showsACardForEachTarget_data()
    {
        QTest::addColumn<int>("slot");
        QTest::addColumn<double>("opacity");
        QTest::newRow("system monitor") << 0 << 0.97;
        QTest::newRow("process monitor over fullscreen") << 1 << 0.7;
        QTest::newRow("router monitor") << 2 << 0.97;
    }

    void showsACardForEachTarget()
    {
        QFETCH(int, slot);
        QFETCH(double, opacity);
        QObject *overlay = create(slot, true);
        QVERIFY(overlay);
        QVERIFY(m_harness->deliver(statePath(), targets(game), [overlay] { return overlay->property("targets").toList().size() == 2; }));
        QCOMPARE(overlay->property("pids").toList(), QVariantList {200});
        QTRY_COMPARE(dialogs(QStringLiteral("Overlay")).size(), 2);
        QObject *card = dialogs(QStringLiteral("Overlay 3")).value(0);
        QVERIFY(card);
        QTRY_VERIFY(card->property("readyToShow").toBool());
        QCOMPARE(card->property("desiredWidth").toDouble(), 120.0);
        QCOMPARE(card->property("title").toString(), QStringLiteral("Konveyor Monitor Overlay 3 %1").arg(slot));
        QObject *compact = loaded(card);
        QVERIFY(compact);
        QVERIFY(compact->property("overlayMode").toBool());
        QCOMPARE(compact->property("overlayTargetPid").toInt(), 200);
        QCOMPARE(compact->property("overlayBackgroundOpacity").toDouble(), opacity);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void opensAndClosesThePopup()
    {
        QObject *overlay = create(1, true);
        QVERIFY(overlay);
        QVERIFY(m_harness->deliver(statePath(), targets(game), [overlay] { return !overlay->property("targets").toList().isEmpty(); }));
        QTRY_COMPARE(dialogs(QStringLiteral("Overlay")).size(), 2);
        QObject *card = dialogs(QStringLiteral("Overlay 3")).value(0);
        QVERIFY(card);
        QObject *popup = dialogs(QStringLiteral("Panel 3")).value(0);
        QVERIFY(popup);
        QTRY_VERIFY(loaded(card));
        QMetaObject::invokeMethod(loaded(card), "overlayClicked");
        QVERIFY(popup->property("visible").toBool());
        QTRY_VERIFY(loaded(popup));
        QVERIFY(loaded(popup)->property("overlayMode").toBool());
        QMetaObject::invokeMethod(loaded(popup), "overlayCloseRequested");
        QVERIFY(!popup->property("visible").toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsCardsAndPopupsOpenWhenOtherTargetsChange()
    {
        QObject *overlay = create(1, true);
        QVERIFY(overlay);
        const QByteArray alone
            = R"([{"key": "game", "pid": 200, "windowId": 3, "x": 0, "y": 0, "width": 900, "height": 600, "fullscreen": true}])";
        QVERIFY(m_harness->deliver(statePath(), targets(alone), [overlay] { return overlay->property("targets").toList().size() == 1; }));
        QTRY_VERIFY(dialogs(QStringLiteral("Overlay 3")).value(0) && loaded(dialogs(QStringLiteral("Overlay 3")).value(0)));
        QPointer<QObject> card = dialogs(QStringLiteral("Overlay 3")).value(0);
        QPointer<QObject> popup = dialogs(QStringLiteral("Panel 3")).value(0);
        QPointer<QObject> compact = loaded(card);
        QMetaObject::invokeMethod(compact, "overlayClicked");
        QTRY_VERIFY(loaded(popup));
        QVERIFY(m_harness->deliver(statePath(), targets(game), [overlay] { return overlay->property("targets").toList().size() == 2; }));
        QTRY_COMPARE(dialogs(QStringLiteral("Overlay")).size(), 2);
        QVERIFY(card && popup && compact);
        QVERIFY(popup->property("visible").toBool());
        QCOMPARE(compact->property("overlayBackgroundOpacity").toDouble(), 0.7);
        QByteArray windowed = game;
        windowed.replace("\"fullscreen\": true", "\"fullscreen\": false");
        QVERIFY(m_harness->deliver(statePath(), targets(windowed),
            [compact] { return compact && compact->property("overlayBackgroundOpacity").toDouble() == 1.0; }));
        QVERIFY(card && popup && popup->property("visible").toBool());
        const QByteArray moved
            = R"([{"key": "game", "pid": 201, "windowId": 5, "x": 0, "y": 0, "width": 900, "height": 600, "fullscreen": false}])";
        QVERIFY(m_harness->deliver(
            statePath(), targets(moved), [overlay] { return overlay->property("pids").toList() == QVariantList {201}; }));
        QTRY_COMPARE(dialogs(QStringLiteral("Overlay")).size(), 1);
        QObject *current = dialogs(QStringLiteral("Overlay 5")).value(0);
        QVERIFY(current);
        QTRY_VERIFY(loaded(current));
        QCOMPARE(loaded(current)->property("overlayTargetPid").toInt(), 201);
        QVERIFY(dialogs(QStringLiteral("Overlay 3")).isEmpty());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void ignoresUnusableState_data()
    {
        QTest::addColumn<QByteArray>("state");
        QTest::newRow("broken json") << QByteArray("{\"targets\": [");
        QTest::newRow("targets not a list") << QByteArray("{\"targets\": {\"pid\": 5}}");
        QTest::newRow("no targets") << QByteArray("{}");
    }

    void ignoresUnusableState()
    {
        QFETCH(QByteArray, state);
        QObject *overlay = create(0, true);
        QVERIFY(overlay);
        QVERIFY(m_harness->deliver(statePath(), targets(game), [overlay] { return !overlay->property("targets").toList().isEmpty(); }));
        QVERIFY(m_harness->deliver(statePath(), state, [overlay] { return overlay->property("targets").toList().isEmpty(); }));
        QTRY_COMPARE(dialogs(QStringLiteral("Overlay")).size(), 0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void staysEmptyWhileInactive()
    {
        m_harness->writeFile(statePath(), targets(game));
        QObject *overlay = create(0, false);
        QVERIFY(overlay);
        QCOMPARE(overlay->property("targets").toList(), QVariantList());
        overlay->setProperty("active", true);
        QTRY_COMPARE(overlay->property("targets").toList().size(), 2);
        overlay->setProperty("active", false);
        QCOMPARE(overlay->property("targets").toList(), QVariantList());
        QTRY_COMPARE(dialogs(QStringLiteral("Overlay")).size(), 0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

private:
    QObject *create(int slot, bool active)
    {
        const QVariantMap properties {{QStringLiteral("slot"), slot}, {QStringLiteral("content"), QVariant::fromValue(m_compact.get())},
            {QStringLiteral("popupContent"), QVariant::fromValue(m_popup.get())}, {QStringLiteral("active"), active}};
        QObject *overlay = m_harness->create(QStringLiteral("contents/ui/lib/MonitorOverlay.qml"), properties);
        if (!overlay) {
            qWarning("%s", qPrintable(m_harness->error));
        }
        return overlay;
    }

    std::unique_ptr<PlasmoidHarness> m_harness;
    std::unique_ptr<QQmlComponent> m_compact;
    std::unique_ptr<QQmlComponent> m_popup;
};

QTEST_MAIN(TestPlasmoidMonitorOverlay)

#include "test_plasmoid_monitoroverlay.moc"
