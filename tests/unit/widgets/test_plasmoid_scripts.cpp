#include "plasmoidharness.h"

#include <QFile>
#include <QQmlComponent>
#include <QQmlExpression>
#include <QTest>

namespace
{

void row(const char *expression, const char *expected)
{
    QTest::newRow(expression) << QString::fromUtf8(expression) << QString::fromUtf8(expected);
}

}

class TestPlasmoidScripts : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void initTestCase()
    {
        m_engine = std::make_unique<QQmlEngine>();
        QFile source(QStringLiteral(KONVEYOR_SOURCE_DIR "/tests/unit/widgets/qmlharness/ScriptProbe.qml"));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QQmlComponent component(m_engine.get());
        component.setData(
            source.readAll(), QUrl::fromLocalFile(PlasmoidHarness::widgetsDir() + QStringLiteral("/shared/common/ScriptProbe.qml")));
        m_probe.reset(component.create());
        QVERIFY2(m_probe, qPrintable(component.errorString()));
    }

    void cleanupTestCase()
    {
        m_probe.reset();
        m_engine.reset();
    }

    void evaluates_data()
    {
        QTest::addColumn<QString>("expression");
        QTest::addColumn<QString>("expected");
        formatRows();
        ringAndHistoryRows();
        stageRows();
        styleRows();
        highlightRows();
    }

    void evaluates()
    {
        QFETCH(QString, expression);
        QFETCH(QString, expected);
        QQmlExpression evaluation(qmlContext(m_probe.get()), m_probe.get(), QStringLiteral("JSON.stringify(%1)").arg(expression));
        const QString result = evaluation.evaluate().toString();
        QVERIFY2(!evaluation.hasError(), qPrintable(evaluation.error().toString()));
        QCOMPARE(result, expected);
    }

    void warnsAboutUnknownStages()
    {
        QTest::ignoreMessage(QtWarningMsg, "PopStage: unknown stage name huge");
        QQmlExpression unknown(qmlContext(m_probe.get()), m_probe.get(), QStringLiteral("Stage.index('huge')"));
        QCOMPARE(unknown.evaluate().toInt(), 3);
        QTest::ignoreMessage(QtWarningMsg, "PopStage: stage index out of range 7");
        QQmlExpression outside(qmlContext(m_probe.get()), m_probe.get(), QStringLiteral("Stage.name(7)"));
        QCOMPARE(outside.evaluate().toString(), QStringLiteral("full"));
        QTest::ignoreMessage(QtWarningMsg, "PopStage: stage index out of range -1");
        QQmlExpression negative(qmlContext(m_probe.get()), m_probe.get(), QStringLiteral("Stage.name(-1)"));
        QCOMPARE(negative.evaluate().toString(), QStringLiteral("full"));
    }

private:
    static void formatRows()
    {
        row("[Fmt.mbps(undefined), Fmt.mbps(null), Fmt.mbps(0), Fmt.mbps(12.34), Fmt.mbps(150), Fmt.mbps(1000), Fmt.mbps(2345)]",
            "[\"0\",\"0\",\"0.0 Mb/s\",\"12.3 Mb/s\",\"150 Mb/s\",\"1.00 Gb/s\",\"2.35 Gb/s\"]");
        row("[Fmt.bytes(undefined), Fmt.bytes(0), Fmt.bytes(1023), Fmt.bytes(1024), Fmt.bytes(10240), Fmt.bytes(1048576), "
            "Fmt.bytes(Math.pow(1024, 4))]",
            "[\"0 KiB\",\"0 KiB\",\"1023 KiB\",\"1.0 MiB\",\"10 MiB\",\"1.0 GiB\",\"1024 TiB\"]");
        row("[Fmt.pct(undefined), Fmt.pct(null), Fmt.pct(12.6)]", "[\"0%\",\"0%\",\"13%\"]");
        row("[Fmt.rate(undefined), Fmt.rate(-1), Fmt.rate(0), Fmt.rate(0.1), Fmt.rate(1), Fmt.rate(125), Fmt.rate(125000)]",
            "[\"—\",\"—\",\"idle\",\"idle\",\"8 b/s\",\"1 Kb/s\",\"1.0 Mb/s\"]");
        row("[Fmt.duration(undefined), Fmt.duration(59), Fmt.duration(3600), Fmt.duration(3661), Fmt.duration(90061)]",
            "[\"0m\",\"0m\",\"1h 0m\",\"1h 1m\",\"1d 1h\"]");
        row("[Fmt.temp(undefined), Fmt.temp(71.5), Fmt.dbm(undefined), Fmt.dbm(-50)]", "[\"0°C\",\"72°C\",\"0 dBm\",\"-50 dBm\"]");
        row("[Fmt.heat(10, 60, 85, theme), Fmt.heat(60, 60, 85, theme), Fmt.heat(85, 60, 85, theme)]", "[\"green\",\"orange\",\"red\"]");
        row("[String(Fmt.grad(undefined)), String(Fmt.grad(0)) === String(Fmt.grad(-5)), String(Fmt.grad(100)) === String(Fmt.grad(500))]",
            "[\"#454bd3\",true,true]");
        row("[Fmt.rssiColor(-60, theme), Fmt.rssiColor(-61, theme), Fmt.rssiColor(-73, theme)]", "[\"green\",\"orange\",\"red\"]");
        row("[-55, -56, -65, -66, -72, -80, -81].map(Fmt.rssiBars)", "[4,3,3,2,2,1,0]");
    }

    static void ringAndHistoryRows()
    {
        row("Ring.values(filled(3, []))", "[]");
        row("Ring.values(filled(3, [1, 2]))", "[1,2]");
        row("Ring.values(filled(3, [1, 2, 3, 4, 5]))", "[3,4,5]");
        row("[Ring.avg(filled(3, [])), Ring.avg(filled(3, [1, 2])), Ring.avg(filled(3, [1, 2, 3, 4, 5]))]", "[0,1.5,4]");
        row("History.values(history(2, 'cpu', [1, 2, 3]), 'cpu')", "[2,3]");
        row("History.values(history(2, 'cpu', [1]), 'gpu')", "[]");
        row("History.values(History.resized(history(4, 'cpu', [1, 2, 3]), 2), 'cpu')", "[2,3]");
        row("History.values(History.resized(history(2, 'cpu', [1, 2]), 5), 'cpu')", "[1,2]");
        row("History.resized(history(2, 'cpu', [1]), 5).len", "5");
        row("[History.stats(undefined), History.stats([]), History.stats([1, 5, 3])]",
            "[{\"now\":0,\"peak\":0,\"avg\":0,\"count\":0},{\"now\":0,\"peak\":0,\"avg\":0,\"count\":0},{\"now\":3,\"peak\":5,\"avg\":3,"
            "\"count\":3}]");
        row("History.stats([-3, -1])", "{\"now\":-1,\"peak\":-1,\"avg\":-2,\"count\":2}");
    }

    static void stageRows()
    {
        row("[Stage.index('tiny'), Stage.index('small'), Stage.index('medium'), Stage.index('full')]", "[0,1,2,3]");
        row("[0, 1, 2, 3].map(Stage.name)", "[\"tiny\",\"small\",\"medium\",\"full\"]");
        row("[Stage.span(chips, 5, 'max'), Stage.span([], 5, 'max'), Stage.span([chips[2]], 5, 'max')]", "[155,0,0]");
        row("Stage.share(1000, chips, 5)", "[30,50,0]");
        row("Stage.share(60, chips, 5)", "[30,25,0]");
        row("Stage.share(30, chips, 5)", "[20,15,0]");
        row("Stage.share(0, chips, 5)", "[20,15,0]");
        row("Stage.share(100, [{ low: 3, high: 3, max: 40, sizes: [1, 2, 3, 40] }, { low: 0, high: 1, max: 9, sizes: [4, 9, 9, 9] }], 0)",
            "[40,9]");
        row("Stage.share(10, [], 5)", "[]");
    }

    static void styleRows()
    {
        row("[Style.bytes(undefined), Style.bytes(1023), Style.bytes(1536), Style.bytes(1048576), Style.bytes(1073741824), "
            "Style.bytes(1099511627776)]",
            "[\"0 B\",\"1023 B\",\"2 KB\",\"1 MB\",\"1.0 GB\",\"1.0 TB\"]");
        row("[Style.isDark(theme), Style.isDark(light)]", "[true,false]");
        row("['temp', 'memory', 'down', 'up', 'power', 'fan'].map(name => String(Style.hue(name, theme))).length", "6");
        row("[Style.hue('other', theme), String(Style.hue('temp', theme)) !== String(Style.hue('temp', light))]", "[\"blue\",true]");
        row("[Style.heat(1, 5, 9, theme), Style.heat(5, 5, 9, theme), Style.heat(9, 5, 9, theme)]", "[\"black\",\"orange\",\"red\"]");
        row("[Style.heatStrong(1, 5, 9, theme), Style.heatStrong(5, 5, 9, theme), Style.heatStrong(9, 5, 9, theme)]",
            "[\"green\",\"orange\",\"red\"]");
    }

    static void highlightRows()
    {
        row("[Highlight.escape(undefined), Highlight.escape(null), Highlight.escape(0), Highlight.escape('<a & b>')]",
            "[\"\",\"\",\"0\",\"&lt;a &amp; b&gt;\"]");
        row("[Highlight.matches('Hello', ''), Highlight.matches('Hello', 'ELL'), Highlight.matches(undefined, 'x'), Highlight.matches(0, "
            "'0')]",
            "[true,true,false,true]");
        row("[Highlight.matchesAny([], ''), Highlight.matchesAny(['a', 'bc'], 'C'), Highlight.matchesAny(['a'], 'z')]",
            "[true,true,false]");
        row("Highlight.mark('Hello', 'LL', '#f00')", "\"He<b><font color=\\\"#f00\\\">ll</font></b>o\"");
        row("[Highlight.mark('a<b', '', 'c'), Highlight.mark('a<b', 'z', 'c'), Highlight.mark(null, 'z', 'c')]",
            "[\"a&lt;b\",\"a&lt;b\",\"\"]");
        row("Highlight.mark('x<y>z', '<y>', 'c')", "\"x<b><font color=\\\"c\\\">&lt;y&gt;</font></b>z\"");
        row("Highlight.mark('İab', 'a', 'c')", "\"İ<b><font color=\\\"c\\\">a</font></b>b\"");
        row("Highlight.mark('a.b', '.', 'c')", "\"a<b><font color=\\\"c\\\">.</font></b>b\"");
    }

    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_probe;
};

QTEST_MAIN(TestPlasmoidScripts)

#include "test_plasmoid_scripts.moc"
