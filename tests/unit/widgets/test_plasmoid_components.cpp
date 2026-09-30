#include "plasmoidharness.h"

#include <QSignalSpy>
#include <QTest>

namespace
{

const PlasmoidSpec anyPlasmoid {QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.panel"),
    QStringLiteral("org.devl0rd.sysmon.panel"), QStringLiteral("cpu"), {}};

}

class TestPlasmoidComponents : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void init()
    {
        m_harness = std::make_unique<PlasmoidHarness>(anyPlasmoid);
        m_harness->setUp(Form::Horizontal);
    }

    void cleanup() { m_harness.reset(); }

    void confirmButtonNeedsASecondClick()
    {
        QObject *button = component("PopConfirm { label: \"Reboot\"; iconName: \"system-reboot\" }");
        QVERIFY(button);
        QSignalSpy confirmed(button, SIGNAL(confirmed()));
        QCOMPARE(button->property("text").toString(), QStringLiteral("Reboot"));
        QMetaObject::invokeMethod(button, "clicked");
        QVERIFY(button->property("armed").toBool());
        QCOMPARE(button->property("text").toString(), QStringLiteral("Click again to confirm"));
        QMetaObject::invokeMethod(button, "clicked");
        QCOMPARE(confirmed.count(), 1);
        QVERIFY(!button->property("armed").toBool());
        QMetaObject::invokeMethod(button, "clicked");
        QTRY_VERIFY_WITH_TIMEOUT(!button->property("armed").toBool(), 10000);
        QCOMPARE(confirmed.count(), 1);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void chipsPickTheirDetailStage_data()
    {
        QTest::addColumn<QString>("properties");
        QTest::addColumn<QString>("stage");
        QTest::newRow("room for everything") << QStringLiteral("fitSpace: 1000") << QStringLiteral("full");
        QTest::newRow("no fit given") << QString() << QStringLiteral("full");
        QTest::newRow("locked") << QStringLiteral("fitSpace: 1000; lockedStage: \"small\"") << QStringLiteral("small");
        QTest::newRow("capped") << QStringLiteral("fitSpace: 1000; cappedStage: \"medium\"") << QStringLiteral("medium");
        QTest::newRow("squeezed bar") << QStringLiteral("fitSpace: 1") << QStringLiteral("tiny");
        QTest::newRow("squeezed with a floor") << QStringLiteral("fitSpace: 1; minimumStage: \"small\"") << QStringLiteral("small");
        QTest::newRow("text can't go tiny") << QStringLiteral("fitSpace: 1; chipStyle: \"text\"") << QStringLiteral("small");
        QTest::newRow("lock below the floor") << QStringLiteral("chipStyle: \"text\"; lockedStage: \"tiny\"") << QStringLiteral("small");
    }

    void chipsPickTheirDetailStage()
    {
        QFETCH(QString, properties);
        QFETCH(QString, stage);
        QObject *chip = component(
            "PopChip { adaptive: true; label: \"CPU\"; value: \"42%\"; fraction: 0.42; panelThickness: 40; " + properties.toUtf8() + " }");
        QVERIFY(chip);
        QCOMPARE(chip->property("stage").toString(), stage);
        const QVariantMap span = chip->property("stageSpan").toMap();
        QVERIFY(span.value(QStringLiteral("min")).toDouble() <= span.value(QStringLiteral("max")).toDouble());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void chipStagesGrowWithTheirContent()
    {
        QObject *chip = component(
            "PopChip { adaptive: true; label: \"GPU\"; value: \"99%\"; secondary: \"70°\"; fraction: 0.99; panelThickness: 40 }");
        QVERIFY(chip);
        const QVariantList sizes = chip->property("stageSizes").toList();
        QCOMPARE(sizes.size(), 4);
        for (int i = 1; i < 3; ++i) {
            QVERIFY2(sizes.at(0).toDouble() < sizes.at(i).toDouble() && sizes.at(i).toDouble() < sizes.at(3).toDouble(),
                qPrintable(QVariant(sizes).toStringList().join(QLatin1Char(','))));
        }
        chip->setProperty("fitSpace", sizes.at(2).toDouble());
        QCOMPARE(chip->property("stage").toString(), QStringLiteral("medium"));
        chip->setProperty("visible", false);
        QCOMPARE(chip->property("stageSpan").toMap().value(QStringLiteral("max")).toDouble(), 0.0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void sparklineFindsItsPeak_data()
    {
        QTest::addColumn<QString>("values");
        QTest::addColumn<int>("index");
        QTest::addColumn<double>("high");
        QTest::newRow("empty") << QStringLiteral("[]") << -1 << 1.0;
        QTest::newRow("one") << QStringLiteral("[5]") << 0 << 5.0;
        QTest::newRow("flat zero") << QStringLiteral("[0, 0]") << 0 << 1.0;
        QTest::newRow("first peak wins") << QStringLiteral("[3, 9, 9, 1]") << 1 << 9.0;
        QTest::newRow("negative") << QStringLiteral("[-5, -2]") << 1 << 1.0;
    }

    void sparklineFindsItsPeak()
    {
        QFETCH(QString, values);
        QFETCH(int, index);
        QFETCH(double, high);
        QObject *line = component("Sparkline { width: 200; height: 60; values: " + values.toUtf8() + " }");
        QVERIFY(line);
        QCOMPARE(line->property("peakIdx").toInt(), index);
        QCOMPARE(line->property("peakHi").toDouble(), high);
        line->setProperty("rangeMax", 100);
        QCOMPARE(line->property("peakHi").toDouble(), 100.0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void cardsAndTabsReportClicks()
    {
        QObject *card = component("PopCard { title: \"CPU\"; collapsible: true; collapsed: true }");
        QVERIFY(card);
        QSignalSpy toggled(card, SIGNAL(collapseToggled()));
        const QList<QObject *> buttons = findByType(card, "ToolButton");
        QVERIFY(!buttons.isEmpty());
        QMetaObject::invokeMethod(buttons.first(), "clicked");
        QCOMPARE(toggled.count(), 1);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void gaugesColourByFill()
    {
        QObject *gauge = component("Gauge { label: \"Disk\"; value: 50 }");
        QVERIFY(gauge);
        QCOMPARE(gauge->property("valueText").toString(), QStringLiteral("50%"));
        const QString half = gauge->property("fillColor").toString();
        gauge->setProperty("value", 100);
        QVERIFY(gauge->property("fillColor").toString() != half);
        gauge->setProperty("useGradient", false);
        QCOMPARE(gauge->property("fillColor"), gauge->property("barColor"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

private:
    QObject *component(const QByteArray &body)
    {
        const QString file = QStringLiteral("contents/ui/lib/Probe%1.qml").arg(++m_probes);
        m_harness->writeFile(m_harness->stagedPath(file), "import QtQuick\n" + body + "\n");
        QObject *object = m_harness->create(file);
        if (!object) {
            qWarning("%s", qPrintable(m_harness->error));
        }
        return object;
    }

    std::unique_ptr<PlasmoidHarness> m_harness;
    int m_probes = 0;
};

QTEST_MAIN(TestPlasmoidComponents)

#include "test_plasmoid_components.moc"
