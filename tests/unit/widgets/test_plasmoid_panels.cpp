#include "procmonfixture.h"
#include "routerfixture.h"

#include <QDateTime>

namespace
{

struct Panel
{
    PlasmoidSpec spec;
    QString path;
    QByteArray snapshot;
    const char *arrived;
};

Panel panel(const QString &name)
{
    const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (name == QLatin1String("sysmon")) {
        return {{QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.panel"), QStringLiteral("org.devl0rd.sysmon.panel"),
                    QStringLiteral("cpu"), {}},
            runtime + QStringLiteral("/Linux-System-Monitor/data.json"), PlasmoidHarness::fixture(QStringLiteral("sysmon.json")),
            "collectorAlive"};
    }
    if (name == QLatin1String("procmon")) {
        return {Procmon::spec, Procmon::runtime() + QStringLiteral("/panel/panel.json"), Procmon::panel, "hasData"};
    }
    if (name == QLatin1String("routermon")) {
        return {Router::variant(QStringLiteral("org.devl0rd.routermon.panel")), Router::runtime() + QStringLiteral("/data.json"),
            Router::snapshot(), "ready"};
    }
    const QByteArray log = "{\"ts\": " + QByteArray::number(QDateTime::currentSecsSinceEpoch())
        + ", \"alive\": true, \"lines\": [{\"t\": " + QByteArray::number(QDateTime::currentMSecsSinceEpoch() * 1000)
        + ", \"p\": 3, \"id\": \"sshd\", \"u\": \"\", \"pid\": \"1\", \"m\": \"x\"}]}";
    return {{QStringLiteral("system-log/plasmoids/org.devl0rd.logmon.journal"), QStringLiteral("org.devl0rd.logmon.journal"),
                QStringLiteral("utilities-log-viewer"), {QStringLiteral("system-log/shared/lib")}},
        runtime + QStringLiteral("/Linux-Log-Monitor/log.json"), log, "collectorOnline"};
}

void addRows(const char *name, const QString &key, const QStringList &values)
{
    for (int form : {Form::Horizontal, Form::Vertical}) {
        for (const QString &value : values) {
            QTest::addRow("%s %s=%s %s", name, qPrintable(key), qPrintable(value), form == Form::Horizontal ? "horizontal" : "vertical")
                << QString::fromLatin1(name) << form << QVariantMap {{key, value}};
        }
    }
}

void addSwitches(const char *name, const QStringList &keys)
{
    for (const QString &key : keys) {
        QTest::addRow("%s without %s", name, qPrintable(key))
            << QString::fromLatin1(name) << int(Form::Horizontal) << QVariantMap {{key, false}};
    }
}

}

class TestPlasmoidPanels : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void drawsEveryPanelSetting_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("form");
        QTest::addColumn<QVariantMap>("config");
        const QStringList details {
            QStringLiteral("auto"), QStringLiteral("full"), QStringLiteral("medium"), QStringLiteral("small"), QStringLiteral("tiny")};
        for (const char *name : {"sysmon", "procmon", "routermon", "logmon"}) {
            addRows(name, QStringLiteral("panelDetail"), details);
        }
        addRows("sysmon", QStringLiteral("compactStyle"), {QStringLiteral("bar"), QStringLiteral("ring"), QStringLiteral("text")});
        addRows("routermon", QStringLiteral("compactExtra"),
            {QStringLiteral("ping"), QStringLiteral("clients"), QStringLiteral("blocked"), QStringLiteral("none")});
        addSwitches("sysmon",
            {QStringLiteral("compactShowCpu"), QStringLiteral("compactShowGpu"), QStringLiteral("compactShowRam"),
                QStringLiteral("compactShowTemps"), QStringLiteral("panelShrink")});
        addSwitches("procmon",
            {QStringLiteral("compactShowCpu"), QStringLiteral("compactShowGpu"), QStringLiteral("compactShowFps"),
                QStringLiteral("panelShrink")});
        addSwitches("routermon", {QStringLiteral("panelShrink")});
        addSwitches("logmon", {QStringLiteral("compactShowWarnings"), QStringLiteral("panelShrink")});
    }

    void drawsEveryPanelSetting()
    {
        QFETCH(QString, name);
        QFETCH(int, form);
        QFETCH(QVariantMap, config);
        const Panel wanted = panel(name);
        PlasmoidHarness harness(wanted.spec);
        QObject *root = harness.load(form, config);
        QVERIFY2(root, qPrintable(harness.error));
        QQuickItem *compact = harness.show("compactRepresentation");
        QVERIFY(compact);
        harness.resolveRuntime(QStringLiteral("printf %s"));
        QVERIFY(harness.deliver(wanted.path, wanted.snapshot, [&] { return root->property(wanted.arrived).toBool(); }));
        QTRY_VERIFY(
            !visibleTexts(compact).join(QString()).isEmpty() || config.value(QStringLiteral("panelDetail")) == QLatin1String("tiny"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
        QFile::remove(wanted.path);
    }
};

QTEST_MAIN(TestPlasmoidPanels)

#include "test_plasmoid_panels.moc"
