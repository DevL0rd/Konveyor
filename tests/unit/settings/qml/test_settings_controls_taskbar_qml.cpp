#include "controldriver.h"

#include <QQmlExpression>
#include <QQmlPropertyMap>

using namespace Konveyor::Settings::Testing;

namespace
{

QObject *taskbarSettings(QQmlEngine &engine)
{
    return engine.singletonInstance<QObject *>(QStringLiteral("org.kde.konveyor.settings"), QStringLiteral("TaskbarSettings"));
}

QVariant valueOf(QQmlEngine &engine, const QString &key)
{
    return taskbarSettings(engine)->property("values").value<QQmlPropertyMap *>()->value(key);
}

}

class TestSettingsControlsTaskbarQml : public QObject
{
    Q_OBJECT

private:
    SettingsHome m_home;

    std::unique_ptr<QObject> openPage(QQmlEngine &engine, const std::optional<QString> &taskbarrc)
    {
        m_home.resetConfig(std::nullopt);
        if (taskbarrc) {
            SettingsHome::write(QDir(m_home.configDir()).filePath(QStringLiteral("taskbarrc")), *taskbarrc);
        }
        QString error;
        std::unique_ptr<QObject> driver = m_home.create(engine, ControlDriver.toByteArray(), &error);
        if (!driver) {
            qWarning("%s", qPrintable(error));
            return driver;
        }
        QVariant opened;
        QMetaObject::invokeMethod(driver.get(), "open", Q_RETURN_ARG(QVariant, opened), Q_ARG(QVariant, QStringLiteral("TaskbarPage")),
            Q_ARG(QVariant, QVariantMap()));
        return opened.toBool() ? std::move(driver) : nullptr;
    }

    static QVariant evaluate(QObject *driver, const QString &expression)
    {
        QQmlExpression evaluation(qmlContext(driver), driver, expression);
        const QVariant result = evaluation.evaluate();
        return evaluation.hasError() ? QVariant(evaluation.error().toString()) : result;
    }

    static QString drive(QObject *driver, const QString &label, const QString &signal, const QByteArray &arguments)
    {
        QVariant problem;
        QMetaObject::invokeMethod(driver, "drive", Q_RETURN_ARG(QVariant, problem), Q_ARG(QVariant, label), Q_ARG(QVariant, 0),
            Q_ARG(QVariant, signal), Q_ARG(QVariant, parsedJson(arguments)));
        return problem.toString();
    }

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void controls_data()
    {
        QTest::addColumn<QString>("label");
        QTest::addColumn<QString>("signal");
        QTest::addColumn<QByteArray>("arguments");
        QTest::addColumn<QString>("key");
        QTest::addColumn<QByteArray>("expected");
        const auto row = [](const char *label, const char *signal, const char *arguments, const char *key, const char *expected) {
            QTest::newRow(label) << QString::fromUtf8(label) << QString::fromLatin1(signal) << QByteArray(arguments)
                                 << QString::fromLatin1(key) << QByteArray(expected);
        };
        row("Show apps", "switched", "[false]", "showApps", "false");
        row("Show workspaces", "switched", "[false]", "showWorkspaces", "false");
        row("Icon size", "chosen", "[false]", "autoIconSize", "false");
        row("Fixed icon size", "edited", "[40.4]", "iconSize", "40");
        row("Space between icons", "edited", "[6]", "iconSpacing", "6");
        row("Padding around each icon", "edited", "[0]", "buttonPadding", "0");
        row("Focused app", "chosen", "[1]", "highlightStyle", "1");
        row("Window indicators", "chosen", "[1]", "indicatorStyle", "1");
        row("Indicator position", "chosen", "[1]", "indicatorEdge", "1");
        row("Pulse apps that want attention", "switched", "[false]", "attentionPulse", "false");
        row("Animations", "switched", "[false]", "animations", "false");
        row("Position", "chosen", "[true]", "workspacesAfterTasks", "true");
        row("Each workspace shows", "chosen", "[2]", "pillContent", "2");
        row("Line between the workspaces and the apps", "switched", "[false]", "showSeparator", "false");
        row("Show empty workspaces", "switched", "[false]", "showEmptyWorkspaces", "false");
        row("Scroll over the workspaces to switch", "switched", "[false]", "wheelSwitchesWorkspaces", "false");
        row("Group an app's windows", "chosen", "[2]", "groupMode", "2");
        row("Only windows on this screen", "switched", "[false]", "onlyThisScreen", "false");
        row("Show floating windows", "switched", "[false]", "showFloating", "false");
        row("Tooltips", "switched", "[false]", "showTooltips", "false");
        row("Clicking the focused app", "chosen", "[2]", "activeClick", "2");
        row("Middle-click", "chosen", "[1]", "middleClick", "1");
        row("Scroll over the apps to switch between them", "switched", "[true]", "wheelCyclesTasks", "true");
        row("Shortcut numbers", "chosen", "[false]", "showShortcutBadges", "false");
        row("Open pinned apps in pin order", "switched", "[false]", "placePinnedLaunches", "false");
        row("Pins", "picked", R"(["org.kde.dolphin"])", "launchers", R"(["applications:org.kde.dolphin.desktop"])");
    }

    void controls()
    {
        QFETCH(QString, label);
        QFETCH(QString, signal);
        QFETCH(QByteArray, arguments);
        QFETCH(QString, key);
        QFETCH(QByteArray, expected);
        WarningLog log;
        QQmlEngine engine;
        const std::unique_ptr<QObject> driver = openPage(engine, std::nullopt);
        QVERIFY(driver);
        QCOMPARE(drive(driver.get(), label, signal, arguments), QString());
        QCOMPARE(jsonText(valueOf(engine, key)), QByteArray("[" + expected + "]"));
        QVERIFY(SettingsHome::read(QDir(m_home.configDir()).filePath(QStringLiteral("taskbarrc"))).contains(key + QLatin1Char('=')));
        QCOMPARE(log.take(), QStringList {});
    }

    void theLastVisiblePartCannotBeHidden()
    {
        QQmlEngine engine;
        const std::unique_ptr<QObject> driver = openPage(engine, QStringLiteral("[General]\nshowWorkspaces=false\n"));
        QVERIFY(driver);
        const QString rowOf = QStringLiteral("items(page).find(item => item.label === '%1').enabled");
        QCOMPARE(evaluate(driver.get(), rowOf.arg(QStringLiteral("Show apps"))).toBool(), false);
        QCOMPARE(evaluate(driver.get(), rowOf.arg(QStringLiteral("Show workspaces"))).toBool(), true);
    }

    void modifiedRowsResetToTheDefault()
    {
        QQmlEngine engine;
        const std::unique_ptr<QObject> driver = openPage(engine, QStringLiteral("[General]\niconSpacing=9\n"));
        QVERIFY(driver);
        const QString row = QStringLiteral("items(page).find(item => item.label === 'Space between icons')");
        QCOMPARE(evaluate(driver.get(), row + QStringLiteral(".modified")).toBool(), true);
        evaluate(driver.get(), row + QStringLiteral(".reset()"));
        QCOMPARE(valueOf(engine, QStringLiteral("iconSpacing")).toInt(), 2);
        QCOMPARE(evaluate(driver.get(), row + QStringLiteral(".modified")).toBool(), false);
    }
};

QTEST_MAIN(TestSettingsControlsTaskbarQml)
#include "test_settings_controls_taskbar_qml.moc"
