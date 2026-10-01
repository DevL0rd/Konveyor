#include <QJsonDocument>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

class TestTaskbarQml : public QObject
{
    Q_OBJECT

    static QString ui() { return QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/taskbar/plasmoid/contents/ui/"); }

    static QVariantMap entry(int index, const QString &app, bool active = false)
    {
        return {{QStringLiteral("index"), index}, {QStringLiteral("appId"), app}, {QStringLiteral("title"), app},
            {QStringLiteral("active"), active}, {QStringLiteral("launcher"), false},
            {QStringLiteral("icon"), QStringLiteral("application-x-executable")}, {QStringLiteral("windowIds"), QStringList {app}},
            {QStringLiteral("launcherUrl"), QStringLiteral("applications:") + app + QStringLiteral(".desktop")}};
    }

private Q_SLOTS:
    void actualDragAndClicksDispatchTheRightTask()
    {
        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.setSource(QUrl::fromLocalFile(ui() + QStringLiteral("TaskbarView.qml")));
        QVERIFY2(view.status() == QQuickView::Ready, qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));
        auto *root = view.rootObject();
        QVERIFY(root);
        root->setProperty(
            "entries", QVariantList {entry(0, QStringLiteral("a")), entry(1, QStringLiteral("b")), entry(2, QStringLiteral("c"), true)});
        view.resize(144, 48);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QVERIFY(QTest::qWaitForWindowActive(&view));
        QTest::qWait(100);
        QSignalSpy move(root, SIGNAL(moveRequested(int, int)));
        QSignalSpy activate(root, SIGNAL(activateRequested(QVariant)));
        QSignalSpy close(root, SIGNAL(closeRequested(QVariant)));
        QTest::mouseMove(&view, QPoint(120, 24), 50);
        QTest::qWait(50);
        QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(120, 24), 50);
        QTest::mouseMove(&view, QPoint(24, 24), 50);
        QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, QPoint(24, 24));
        QTRY_COMPARE(move.count(), 1);
        QCOMPARE(move.first().at(0).toInt(), 2);
        QCOMPARE(move.first().at(1).toInt(), 0);
        QCOMPARE(activate.count(), 0);
        QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, QPoint(120, 24));
        QTRY_COMPARE(activate.count(), 1);
        QCOMPARE(activate.first().first().toMap().value(QStringLiteral("appId")).toString(), QStringLiteral("c"));
        QTest::mouseClick(&view, Qt::MiddleButton, Qt::NoModifier, QPoint(72, 24));
        QTRY_COMPARE(close.count(), 1);
        QCOMPARE(close.first().first().toMap().value(QStringLiteral("appId")).toString(), QStringLiteral("b"));
    }

    void launchesPreserveInterleavedGroupsUntilAnExplicitIconEdit()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(
            "import QtQml\nimport \"Order.js\" as Order\nQtObject { function ordered(preserve) { return Order.groups(["
            "{group:true,windowIds:['a1','a2']},{windowIds:['b']},{windowIds:['c']}], ["
            "{task_id:'a1',id:1,output:'DP-1',taskbar_eligible:true,layout:{pos_in_scrolling_layout:[1,1]}},"
            "{task_id:'b',id:2,output:'DP-1',taskbar_eligible:true,layout:{pos_in_scrolling_layout:[2,1]}},"
            "{task_id:'a2',id:3,output:'DP-1',taskbar_eligible:true,layout:{pos_in_scrolling_layout:[3,1]}},"
            "{task_id:'c',id:4,output:'DP-1',taskbar_eligible:true,layout:{pos_in_scrolling_layout:[4,1]}}], 'DP-1', preserve) } }",
            QUrl::fromLocalFile(ui() + QStringLiteral("launch.qml")));
        const std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        for (const bool preserve : {true, false}) {
            QVariant result;
            QVERIFY(QMetaObject::invokeMethod(root.get(), "ordered", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, preserve)));
            const auto value = result.value<QJSValue>().toVariant();
            QCOMPARE(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact),
                preserve ? QByteArray("[[],[2],[4]]") : QByteArray("[[1,3],[2],[4]]"));
        }
    }

    void taskIdsAreNormalizedDeduplicatedAndScoped()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQml\nimport \"Order.js\" as Order\nQtObject { function ordered(entries, windows) { return "
                          "Order.groups(entries, windows, 'DP-1') } }",
            QUrl::fromLocalFile(ui() + QStringLiteral("probe.qml")));
        QTRY_VERIFY(component.isReady() || component.isError());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        const std::unique_ptr<QObject> root(component.create());
        QVERIFY(root);
        const QVariantList entries {QVariantMap {{QStringLiteral("windowIds"), QStringList {QStringLiteral("ABC"), QStringLiteral("def")}}},
            QVariantMap {{QStringLiteral("windowIds"), QStringList {QStringLiteral("abc"), QStringLiteral("xyz")}}}};
        const QVariantList windows {QVariantMap {{QStringLiteral("task_id"), QStringLiteral("{abc}")}, {QStringLiteral("id"), 1},
                                        {QStringLiteral("output"), QStringLiteral("DP-1")}},
            QVariantMap {{QStringLiteral("task_id"), QStringLiteral("{def}")}, {QStringLiteral("id"), 2},
                {QStringLiteral("output"), QStringLiteral("DP-1")}},
            QVariantMap {{QStringLiteral("task_id"), QStringLiteral("{xyz}")}, {QStringLiteral("id"), 3},
                {QStringLiteral("output"), QStringLiteral("DP-2")}}};
        QVariant result;
        QVERIFY(QMetaObject::invokeMethod(
            root.get(), "ordered", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, entries), Q_ARG(QVariant, windows)));
        const auto value = result.value<QJSValue>().toVariant();
        QCOMPARE(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QByteArray("[[1,2],[]]"));
    }
};

QTEST_MAIN(TestTaskbarQml)
#include "test_taskbar_qml.moc"
