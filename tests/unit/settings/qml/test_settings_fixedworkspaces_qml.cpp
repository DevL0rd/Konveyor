#include "controldriver.h"

#include <QPalette>
#include <QQuickWindow>

using namespace Konveyor::Settings::Testing;

class TestSettingsFixedWorkspacesQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
        QVERIFY(m_home.setUp());
    }

    void previewAndControls_data()
    {
        QTest::addColumn<int>("width");
        QTest::addColumn<bool>("dark");
        QTest::addColumn<QString>("names");
        const QString names = QStringLiteral("workspace \"Web\"\nworkspace \"Chat\"\n");
        QString many;
        for (int i = 0; i < 12; ++i) {
            many += QStringLiteral("workspace \"Long named workspace for project %1 and collaboration\"\n").arg(i);
        }
        QTest::newRow("compact-light") << 720 << false << names;
        QTest::newRow("wide-light") << 1100 << false << names;
        QTest::newRow("compact-dark") << 720 << true << names;
        QTest::newRow("wide-dark") << 1100 << true << names;
        QTest::newRow("empty-light") << 720 << false << QString();
        QTest::newRow("empty-dark") << 1100 << true << QString();
        QTest::newRow("many-light") << 720 << false << many;
        QTest::newRow("many-dark") << 1100 << true << many;
    }

    void previewAndControls()
    {
        QFETCH(int, width);
        QFETCH(bool, dark);
        QFETCH(QString, names);
        QPalette palette;
        palette.setColor(QPalette::Window, dark ? QColor("#252525") : QColor("#f0f0f0"));
        palette.setColor(QPalette::Base, dark ? QColor("#1b1b1b") : QColor("#ffffff"));
        palette.setColor(QPalette::Text, dark ? QColor("#f0f0f0") : QColor("#202020"));
        palette.setColor(QPalette::WindowText, palette.color(QPalette::Text));
        palette.setColor(QPalette::Button, palette.color(QPalette::Window));
        palette.setColor(QPalette::ButtonText, palette.color(QPalette::Text));
        QGuiApplication::setPalette(palette);
        m_home.resetConfig(QStringLiteral("layout { fixed-workspace-count 4; }\n") + names);
        WarningLog log;
        QQmlEngine engine;
        QString error;
        QByteArray driver(ControlDriver.data(), ControlDriver.size());
        driver.replace(
            "import QtQuick.Window", "import QtQuick.Window\nimport org.kde.kirigami as Kirigami\nimport QtQuick.Controls as QQC2");
        driver.replace("Window {",
            "Window {\n"
            "    function applyTheme(dark) {\n"
            "        if (!dark) {\n"
            "            return;\n"
            "        }\n"
            "        for (const item of items(page)) {\n"
            "            if (item instanceof Item) {\n"
            "                item.Kirigami.Theme.inherit = false;\n"
            "                item.Kirigami.Theme.backgroundColor = \"#252525\";\n"
            "                item.Kirigami.Theme.alternateBackgroundColor = \"#303030\";\n"
            "                item.Kirigami.Theme.textColor = \"#f0f0f0\";\n"
            "                item.Kirigami.Theme.disabledTextColor = \"#a0a0a0\";\n"
            "                if (item instanceof QQC2.Control) {\n"
            "                    item.palette.window = \"#252525\";\n"
            "                    item.palette.windowText = \"#f0f0f0\";\n"
            "                    item.palette.base = \"#1b1b1b\";\n"
            "                    item.palette.text = \"#f0f0f0\";\n"
            "                    item.palette.button = \"#303030\";\n"
            "                    item.palette.buttonText = \"#f0f0f0\";\n"
            "                }\n"
            "                if (item instanceof QQC2.Label) {\n"
            "                    item.color = \"#f0f0f0\";\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "        color = \"#252525\";\n"
            "    }\n");
        const auto root = m_home.create(engine, driver, &error);
        QVERIFY2(root, qPrintable(error));
        auto *window = qobject_cast<QQuickWindow *>(root.get());
        window->setWidth(width);
        QVariant opened;
        QVERIFY(QMetaObject::invokeMethod(root.get(), "open", Q_RETURN_ARG(QVariant, opened),
            Q_ARG(QVariant, QStringLiteral("WorkspacesPage")), Q_ARG(QVariant, QVariantMap {})));
        QVERIFY(opened.toBool());
        QVERIFY(QMetaObject::invokeMethod(root.get(), "applyTheme", Q_ARG(QVariant, dark)));
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QObject *page = root->property("page").value<QObject *>();
        QVERIFY(page);
        QCOMPARE(page->property("fixedCount").toInt(), 4);
        const auto capture = [&](const QString &mode) {
            QTest::qWait(250);
            QCoreApplication::processEvents();
            QMetaObject::invokeMethod(root.get(), "applyTheme", Q_ARG(QVariant, dark));
            window->grabWindow();
            const QImage image = window->grabWindow();
            if (const QString directory = qEnvironmentVariable("KONVEYOR_SETTINGS_SCREENSHOTS"); !directory.isEmpty()) {
                return image.save(
                    directory + QStringLiteral("/workspaces-%1-%2.png").arg(mode, QString::fromLatin1(QTest::currentDataTag())));
            }
            return !image.isNull();
        };
        QVERIFY(capture(QStringLiteral("fixed")));
        QVariant result;
        QVERIFY(QMetaObject::invokeMethod(root.get(), "drive", Q_RETURN_ARG(QVariant, result),
            Q_ARG(QVariant, QStringLiteral("Use a fixed number of workspaces")), Q_ARG(QVariant, 0),
            Q_ARG(QVariant, QStringLiteral("switched")), Q_ARG(QVariant, QVariantList {false})));
        QCOMPARE(result.toString(), QString());
        QTRY_COMPARE(page->property("fixedCount").toInt(), 0);
        QVERIFY(capture(QStringLiteral("dynamic")));
        QCOMPARE(log.take(), QStringList {});
    }

private:
    SettingsHome m_home;
};

QTEST_MAIN(TestSettingsFixedWorkspacesQml)
#include "test_settings_fixedworkspaces_qml.moc"
