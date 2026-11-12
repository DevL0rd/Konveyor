#include "plasmoidharness.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

namespace
{

const PlasmoidSpec taskbar {QStringLiteral("taskbar/plasmoids/org.devl0rd.taskbar"), QStringLiteral("org.devl0rd.taskbar"),
    QStringLiteral("preferences-system-windows"), {}};

const QByteArray probe = R"(import QtQuick

Item {
    id: probe

    function window(name, active) {
        return { uuid: name, title: name + " window", appName: name, icon: "utilities-terminal", active: active, minimized: false, attention: false }
    }

    function find(item, test) {
        if (test(item))
            return item
        for (const child of item.children) {
            const found = find(child, test)
            if (found)
                return found
        }
        return null
    }

    function settle(item) {
        for (const child of item.children)
            settle(child)
        if (item.forceLayout !== undefined)
            item.forceLayout()
    }

    function measure(mode, vertical, thickness, name, members) {
        look.nameMode = mode
        look.vertical = vertical
        look.thickness = thickness
        const windows = []
        for (let i = 0; i < members; ++i)
            windows.push(window(name, i === 0))
        entry.entry = { kind: "column", key: "c1", tabbed: false, windows: windows }
        settle(entry)
        const button = find(entry, item => item.named !== undefined && item.visible)
        const label = find(button, item => item.elide === Text.ElideRight && item.maximumLineCount === 1)
        const icon = find(button, item => item.source !== undefined && item.active !== undefined)
        return JSON.stringify({ width: entry.width, height: entry.height, button: look.button, iconSize: look.iconSize,
            named: button.named, label: label.visible, labelWidth: label.width, truncated: label.truncated,
            icon: icon.visible, buttonWidth: button.width, shellWidth: entry.children[0].width })
    }

    TaskLook {
        id: look
        nameWidth: 140
    }

    TaskEntry {
        id: entry
        look: look
    }
}
)";

}

class TestPlasmoidTaskbarNames : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void init()
    {
        m_harness = std::make_unique<PlasmoidHarness>(taskbar);
        m_harness->setUp(Form::Horizontal);
        const QString file = QStringLiteral("contents/ui/NamesProbe.qml");
        m_harness->writeFile(m_harness->stagedPath(file), probe);
        QVERIFY2(m_harness->create(file), qPrintable(m_harness->error));
    }

    void cleanup() { m_harness.reset(); }

    void iconsOnlyKeepsTheSquareButton()
    {
        const QJsonObject shown = measure(0, false, 44, "Konsole");
        QVERIFY(!shown[QStringLiteral("named")].toBool());
        QVERIFY(!shown[QStringLiteral("label")].toBool());
        QVERIFY(shown[QStringLiteral("icon")].toBool());
        QCOMPARE(shown[QStringLiteral("width")].toDouble(), shown[QStringLiteral("button")].toDouble());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void iconAndNameWidensTheButton()
    {
        const QJsonObject shown = measure(1, false, 44, "Konsole");
        QVERIFY(shown[QStringLiteral("named")].toBool());
        QVERIFY(shown[QStringLiteral("label")].toBool());
        QVERIFY(shown[QStringLiteral("icon")].toBool());
        QVERIFY(!shown[QStringLiteral("truncated")].toBool());
        QVERIFY(shown[QStringLiteral("width")].toDouble()
            > shown[QStringLiteral("button")].toDouble() + shown[QStringLiteral("labelWidth")].toDouble() - 1);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void nameOnlyHidesTheIcon()
    {
        const QJsonObject shown = measure(2, false, 44, "Konsole");
        QVERIFY(shown[QStringLiteral("label")].toBool());
        QVERIFY(!shown[QStringLiteral("icon")].toBool());
        QVERIFY(shown[QStringLiteral("width")].toDouble() < measure(1, false, 44, "Konsole")[QStringLiteral("width")].toDouble());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void longNamesEndInAnEllipsis()
    {
        const QJsonObject shown = measure(1, false, 44, "A Very Long Application Name That Keeps Going On And On");
        QVERIFY(shown[QStringLiteral("truncated")].toBool());
        QCOMPARE(shown[QStringLiteral("labelWidth")].toDouble(), 140.0);
        m_harness->eval(QStringLiteral("look.nameWidth = 60"));
        QCOMPARE(measure(1, false, 44, "A Very Long Application Name")[QStringLiteral("labelWidth")].toDouble(), 60.0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void narrowSidePanelsKeepIcons()
    {
        for (int mode : {1, 2}) {
            const QJsonObject shown = measure(mode, true, 44, "Konsole");
            QVERIFY(!shown[QStringLiteral("named")].toBool());
            QVERIFY(shown[QStringLiteral("icon")].toBool());
            QCOMPARE(shown[QStringLiteral("height")].toDouble(), shown[QStringLiteral("button")].toDouble());
        }
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void wideSidePanelsFitTheNameBesideTheIcon()
    {
        const QJsonObject shown = measure(1, true, 200, "A Very Long Application Name That Keeps Going On And On");
        QVERIFY(shown[QStringLiteral("named")].toBool());
        QVERIFY(shown[QStringLiteral("icon")].toBool());
        QVERIFY(shown[QStringLiteral("truncated")].toBool());
        QVERIFY(shown[QStringLiteral("labelWidth")].toDouble() < 200 - shown[QStringLiteral("iconSize")].toDouble());
        QCOMPARE(shown[QStringLiteral("width")].toDouble(), 200.0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void sharedColumnsNameEveryWindow()
    {
        const QJsonObject flat = measure(1, false, 44, "Konsole", 2);
        QVERIFY(flat[QStringLiteral("named")].toBool());
        QVERIFY(flat[QStringLiteral("width")].toDouble() > flat[QStringLiteral("buttonWidth")].toDouble() * 2);
        const QJsonObject side = measure(1, true, 200, "Konsole", 2);
        QCOMPARE(side[QStringLiteral("shellWidth")].toDouble(), 200.0);
        const QJsonObject narrow = measure(1, true, 44, "Konsole", 2);
        QVERIFY(narrow[QStringLiteral("shellWidth")].toDouble() < 44);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

private:
    QJsonObject measure(int mode, bool vertical, int thickness, const char *name, int members = 1)
    {
        const QVariant result = m_harness->eval(QStringLiteral("measure(%1, %2, %3, \"%4\", %5)")
                .arg(mode)
                .arg(vertical ? QStringLiteral("true") : QStringLiteral("false"))
                .arg(thickness)
                .arg(QLatin1String(name))
                .arg(members));
        return QJsonDocument::fromJson(result.toString().toUtf8()).object();
    }

    std::unique_ptr<PlasmoidHarness> m_harness;
};

QTEST_MAIN(TestPlasmoidTaskbarNames)

#include "test_plasmoid_taskbar_names.moc"
