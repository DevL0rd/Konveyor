#include "plasmoidharness.h"

#include <QTest>

namespace
{

const PlasmoidSpec screenRotate {
    QStringLiteral("screen-rotate/plasmoid"), QStringLiteral("dev.devl0rd.screenrotate"), QStringLiteral("object-rotate-left"), {}};

QString helper(const char *mode)
{
    return qEnvironmentVariable("HOME") + QStringLiteral("/.local/bin/linux-plasma-screen-rotate ") + QLatin1String(mode);
}

std::unique_ptr<PlasmoidHarness> started(int form)
{
    return PlasmoidHarness::started(screenRotate, form);
}

QObject *rotateButton(PlasmoidHarness &harness)
{
    const QList<QObject *> buttons = harness.findAll("Button");
    for (QObject *button : buttons) {
        if (button->property("text").toString().startsWith(QLatin1String("Rotate to"))) {
            return button;
        }
    }
    return nullptr;
}

}

class TestPlasmoidScreenRotate : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void readsTheRotationOnStart_data()
    {
        QTest::addColumn<int>("form");
        QTest::addColumn<QString>("state");
        QTest::addColumn<bool>("portrait");
        QTest::newRow("desktop portrait") << Form::Planar << QStringLiteral("portrait\n") << true;
        QTest::newRow("panel landscape") << Form::Horizontal << QStringLiteral("landscape\n") << false;
        QTest::newRow("vertical panel portrait") << Form::Vertical << QStringLiteral("portrait\n") << true;
    }

    void readsTheRotationOnStart()
    {
        QFETCH(int, form);
        QFETCH(QString, state);
        QFETCH(bool, portrait);
        auto harness = started(form);
        QVERIFY(harness);
        QObject *root = harness->root();
        QCOMPARE(root->property("stateText").toString(), QStringLiteral("Checking…"));
        QCOMPARE(harness->command(helper("state")), helper("state"));
        QVERIFY(harness->reply(helper("state"), state));
        QVERIFY(root->property("known").toBool());
        QCOMPARE(root->property("portrait").toBool(), portrait);
        QCOMPARE(root->property("stateText").toString(), portrait ? QStringLiteral("Portrait") : QStringLiteral("Landscape"));
        QCOMPARE(harness->plasmoid()->icon, portrait ? QStringLiteral("object-rotate-right") : QStringLiteral("object-rotate-left"));
        QCOMPARE(harness->plasmoid()->title, QStringLiteral("Screen Rotate"));
        QCOMPARE(root->property("toolTipSubText").toString(),
            portrait ? QStringLiteral("Portrait - click for landscape") : QStringLiteral("Landscape - click for portrait"));
        QCOMPARE(rotateButton(*harness)->property("text").toString(),
            portrait ? QStringLiteral("Rotate to landscape") : QStringLiteral("Rotate to portrait"));
        const bool compact = form != Form::Planar;
        QCOMPARE(root->property("preferredRepresentation").value<QQmlComponent *>(),
            root->property(compact ? "compactRepresentation" : "fullRepresentation").value<QQmlComponent *>());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void togglesOnceAndRereadsTheState()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(harness->reply(helper("state"), QStringLiteral("landscape\n")));
        QMetaObject::invokeMethod(harness->root(), "toggleRotation");
        QVERIFY(harness->root()->property("busy").toBool());
        QVERIFY(!rotateButton(*harness)->property("enabled").toBool());
        QMetaObject::invokeMethod(harness->root(), "toggleRotation");
        QCOMPARE(DataSourceDouble::commands().count(helper("toggle")), 1);
        QVERIFY(harness->reply(helper("toggle"), QStringLiteral("done\n")));
        QVERIFY(harness->root()->property("busy").toBool());
        QCOMPARE(DataSourceDouble::commands().count(helper("state")), 2);
        QVERIFY(harness->reply(helper("state"), QStringLiteral("portrait\n")));
        QVERIFY(!harness->root()->property("busy").toBool());
        QVERIFY(harness->root()->property("portrait").toBool());
        QVERIFY(rotateButton(*harness)->property("enabled").toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void theButtonRotates()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(harness->reply(helper("state"), QStringLiteral("portrait\n")));
        QMetaObject::invokeMethod(rotateButton(*harness), "clicked");
        QCOMPARE(harness->command(helper("toggle")), helper("toggle"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsWhenTheHelperFails()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        const QString message = QStringLiteral("linux-plasma-screen-rotate: kscreen-doctor could not read the screens");
        QVERIFY(harness->reply(helper("state"), QString(), 1, message + QLatin1Char('\n')));
        QObject *root = harness->root();
        QCOMPARE(root->property("stateText").toString(), QStringLiteral("Rotation failed"));
        QCOMPARE(root->property("toolTipSubText").toString(), message);
        QVERIFY(!root->property("known").toBool());
        QVERIFY(visibleTexts(harness->scene()).contains(QStringLiteral("Rotation failed")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void aFailedToggleKeepsTheStateAndCanBeRetried()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(harness->reply(helper("state"), QStringLiteral("landscape\n")));
        QMetaObject::invokeMethod(harness->root(), "toggleRotation");
        QVERIFY(harness->reply(helper("toggle"), QString(), 1));
        QObject *root = harness->root();
        QVERIFY(!root->property("busy").toBool());
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("The screen rotate helper failed"));
        QCOMPARE(DataSourceDouble::commands().count(helper("state")), 1);
        QMetaObject::invokeMethod(root, "toggleRotation");
        QVERIFY(harness->reply(helper("toggle"), QStringLiteral("done\n")));
        QVERIFY(harness->reply(helper("state"), QStringLiteral("portrait\n")));
        QCOMPARE(root->property("stateText").toString(), QStringLiteral("Portrait"));
        QCOMPARE(root->property("failure").toString(), QString());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidScreenRotate)

#include "test_plasmoid_screenrotate.moc"
