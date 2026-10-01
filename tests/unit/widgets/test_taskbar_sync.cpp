#include <QJsonDocument>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>

class TestTaskbarSync : public QObject
{
    Q_OBJECT

    static QVariant json(const QByteArray &text) { return QJsonDocument::fromJson(text).toVariant(); }

    static QVariantMap plan(
        const QByteArray &entries, const QByteArray &windows, const QString &mode = QStringLiteral("follow"), int fallback = 1)
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(
            "import QtQml\nimport \"Sync.js\" as Sync\nQtObject { function plan(entries, windows, mode, fallback) { "
            "const scoped = Sync.scoped(windows, 'DP-1', 1); return {grouping: Sync.grouping(entries, scoped, mode, fallback), "
            "reverse: Sync.reverse(entries, scoped), pinned: Sync.pinned(entries, ['applications:a.desktop','applications:b.desktop']), "
            "ready: Sync.ready(entries, scoped)} } }",
            QUrl::fromLocalFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/taskbar/plasmoid/contents/ui/plan.qml")));
        const std::unique_ptr<QObject> root(component.create());
        if (!root) {
            qWarning() << component.errorString();
            return {};
        }
        QVariant result;
        const bool invoked = QMetaObject::invokeMethod(root.get(), "plan", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, json(entries)),
            Q_ARG(QVariant, json(windows)), Q_ARG(QVariant, mode), Q_ARG(QVariant, fallback));
        return invoked ? result.value<QJSValue>().toVariant().toMap() : QVariantMap();
    }

private Q_SLOTS:
    void followGroupingRespectsConflictsAndOverrides()
    {
        const QByteArray entries = R"([{"appId":"a","windowIds":["a1"]},{"appId":"a","windowIds":["a2"]}])";
        const QByteArray windows = R"([{"task_id":"a1","output":"DP-1","workspace_id":1,"taskbar_grouping":1},
            {"task_id":"a2","output":"DP-1","workspace_id":1,"taskbar_grouping":0}])";
        const auto followed = plan(entries, windows).value(QStringLiteral("grouping")).toMap();
        QCOMPARE(followed.value(QStringLiteral("blacklist")).toStringList(), QStringList {QStringLiteral("a")});
        QCOMPARE(plan(entries, windows, QStringLiteral("separate"))
                     .value(QStringLiteral("grouping"))
                     .toMap()
                     .value(QStringLiteral("grouped"))
                     .toBool(),
            false);
        QCOMPARE(plan(entries, windows, QStringLiteral("grouped"))
                     .value(QStringLiteral("grouping"))
                     .toMap()
                     .value(QStringLiteral("blacklist"))
                     .toList()
                     .size(),
            0);
        const auto off = plan("[]", "[]", QStringLiteral("follow"), 0).value(QStringLiteral("grouping")).toMap();
        QCOMPARE(off.value(QStringLiteral("grouped")).toBool(), false);
    }

    void reverseProjectsFirstEligibleColumnAndPreservesExcludedPositions()
    {
        const QByteArray entries = R"([{"appId":"a","windowIds":["a1","a2"],"launcherUrl":"applications:a.desktop"},
            {"appId":"float","windowIds":["f"]},{"appId":"b","windowIds":["b"],"launcherUrl":"applications:b.desktop"}])";
        const QByteArray windows
            = R"([{"task_id":"a1","output":"DP-1","workspace_id":1,"taskbar_eligible":true,"layout":{"pos_in_scrolling_layout":[3,1]}},
            {"task_id":"a2","output":"DP-1","workspace_id":1,"taskbar_eligible":true,"layout":{"pos_in_scrolling_layout":[4,1]}},
            {"task_id":"b","output":"DP-1","workspace_id":1,"taskbar_eligible":true,"layout":{"pos_in_scrolling_layout":[1,1]}},
            {"task_id":"f","output":"DP-1","workspace_id":1,"taskbar_eligible":false}])";
        const auto result = plan(entries, windows);
        QCOMPARE(result.value(QStringLiteral("reverse")).toList(), (QVariantList {2, 1, 0}));
        QCOMPARE(result.value(QStringLiteral("pinned")).toList(), (QVariantList {0, 1, 2}));
        QCOMPARE(result.value(QStringLiteral("ready")).toBool(), true);
    }

    void otherScopesAndUnlistedWindowsCannotDriveReverseOrdering()
    {
        const auto result = plan(R"([{"windowIds":["a"]}])",
            R"([{"task_id":"a","output":"DP-2","workspace_id":1,"taskbar_eligible":true},
                {"task_id":"x","output":"DP-1","workspace_id":2,"taskbar_eligible":true},
                {"task_id":"unlisted","output":"DP-1","workspace_id":1,"taskbar_eligible":true}])");
        QCOMPARE(result.value(QStringLiteral("reverse")).toList(), QVariantList {0});
        QCOMPARE(result.value(QStringLiteral("ready")).toBool(), false);
    }
};

QTEST_MAIN(TestTaskbarSync)
#include "test_taskbar_sync.moc"
