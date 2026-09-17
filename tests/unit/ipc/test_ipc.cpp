#include "ipc/model.h"

#include <QTest>

using namespace Konveyor;

class TestIpc : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesPositionalArgumentsAndProperties()
    {
        const auto request = Ipc::actionFromArguments(
            {QStringLiteral("move-column-to-workspace"), QStringLiteral("2"), QStringLiteral("--focus"), QStringLiteral("false")});
        QVERIFY(request.has_value());
        QCOMPARE(request->action.name, QStringLiteral("move-column-to-workspace"));
        QCOMPARE(request->action.arguments, QStringList {QStringLiteral("2")});
        QCOMPARE(request->action.properties.size(), 1);
        QCOMPARE(request->action.properties.first().first, QStringLiteral("focus"));
        QCOMPARE(request->action.properties.first().second, QStringLiteral("false"));
        QVERIFY(!request->target.has_value());
    }

    void parsesWindowIdTarget()
    {
        const auto request = Ipc::actionFromArguments({QStringLiteral("close-window"), QStringLiteral("--id"), QStringLiteral("42")});
        QVERIFY(request.has_value());
        QCOMPARE(request->target.value(), Layout::WindowId {42});
        QVERIFY(request->action.properties.isEmpty());
    }

    void keepsNegativeSizeChangeAsArgument()
    {
        const auto request = Ipc::actionFromArguments({QStringLiteral("set-column-width"), QStringLiteral("-10%")});
        QVERIFY(request.has_value());
        QCOMPARE(request->action.arguments, QStringList {QStringLiteral("-10%")});
    }

    void rejectsInvalidInput()
    {
        QVERIFY(!Ipc::actionFromArguments({}).has_value());
        QVERIFY(!Ipc::actionFromArguments({QStringLiteral("focus-window"), QStringLiteral("--id")}).has_value());
        QVERIFY(!Ipc::actionFromArguments({QStringLiteral("focus-window"), QStringLiteral("--id"), QStringLiteral("abc")}).has_value());
        QVERIFY(!Ipc::actionFromJson({}).has_value());
    }

    void actionJsonRoundTrips()
    {
        Ipc::ActionRequest request;
        request.action = {QStringLiteral("move-floating-window"), {},
            {{QStringLiteral("x"), QStringLiteral("+10")}, {QStringLiteral("y"), QStringLiteral("-5%")}}};
        request.target = 7;
        const auto parsed = Ipc::actionFromJson(Ipc::actionToJson(request));
        QVERIFY(parsed.has_value());
        QCOMPARE(parsed->action.name, request.action.name);
        QCOMPARE(parsed->target, request.target);
        QCOMPARE(parsed->action.properties.size(), 2);
    }

    void windowJsonUsesIpcFieldNames()
    {
        Layout::WindowState state;
        state.id = 3;
        state.workspace = 9;
        state.isFocused = true;
        state.columnIndex = 1;
        state.tileIndex = 0;
        state.targetFrame = QRectF(10, 20, 300, 400);
        const QJsonObject json = Ipc::windowToJson({3, QStringLiteral("Title"), QStringLiteral("app"), 1234}, state);
        QCOMPARE(json.value(QStringLiteral("app_id")).toString(), QStringLiteral("app"));
        QCOMPARE(json.value(QStringLiteral("workspace_id")).toInteger(), 9);
        QCOMPARE(json.value(QStringLiteral("is_focused")).toBool(), true);
        const QJsonObject layout = json.value(QStringLiteral("layout")).toObject();
        QCOMPARE(layout.value(QStringLiteral("pos_in_scrolling_layout")).toArray(), (QJsonArray {2, 1}));
        QCOMPARE(layout.value(QStringLiteral("tile_size")).toArray(), (QJsonArray {300.0, 400.0}));
    }

    void floatingWindowHasNoScrollingPosition()
    {
        Layout::WindowState state;
        state.isFloating = true;
        const QJsonObject json = Ipc::windowToJson({}, state);
        QVERIFY(json.value(QStringLiteral("layout")).toObject().value(QStringLiteral("pos_in_scrolling_layout")).isNull());
    }

    void gesturesJsonListsOnlyActiveGestures()
    {
        Config::Gestures gestures;
        gestures.touchpad.verticalSwipe = Config::VerticalSwipe::Off;
        gestures.touchpad.windowHorizontalSwipe = Config::WindowHorizontalSwipe::Off;
        gestures.touchscreen.windowVerticalSwipe = Config::WindowVerticalSwipe::Off;
        gestures.touchscreen.pinchFingers = 5;
        gestures.touchscreen.longPressMs = 700;
        gestures.touchscreen.threeFingerTap = Config::TapAction::Off;
        gestures.touchscreen.fourFingerTap = Config::TapAction::Off;
        gestures.touchscreen.fiveFingerTap = Config::TapAction::ToggleOverview;
        const QJsonArray array = Ipc::gesturesToJson(gestures);
        QCOMPARE(array.size(), 11);
        QCOMPARE(array[0].toObject()[QStringLiteral("motion")].toString(), QStringLiteral("swipe-horizontal"));
        QCOMPARE(array[1].toObject()[QStringLiteral("motion")].toString(), QStringLiteral("window-swipe-vertical"));
        QCOMPARE(array[2].toObject()[QStringLiteral("motion")].toString(), QStringLiteral("pinch"));
        QCOMPARE(array[2].toObject()[QStringLiteral("fingers")].toInt(), 4);
        QCOMPARE(array[3].toObject()[QStringLiteral("motion")].toString(), QStringLiteral("tap"));
        QCOMPARE(array[3].toObject()[QStringLiteral("fingers")].toInt(), 3);
        QCOMPARE(array[3].toObject()[QStringLiteral("action")].toString(), QStringLiteral("cycle-width"));
        QCOMPARE(array[4].toObject()[QStringLiteral("action")].toString(), QStringLiteral("kontrol-panel"));
        QCOMPARE(array[7].toObject()[QStringLiteral("action")].toString(), QStringLiteral("consume-or-expel"));
        QCOMPARE(array[8].toObject()[QStringLiteral("fingers")].toInt(), 5);
        QCOMPARE(array[9].toObject()[QStringLiteral("fingers")].toInt(), 5);
        QCOMPARE(array[9].toObject()[QStringLiteral("action")].toString(), QStringLiteral("toggle-overview"));
        QCOMPARE(array[10].toObject()[QStringLiteral("action")].toString(), QStringLiteral("move-window"));
        QCOMPARE(array[10].toObject()[QStringLiteral("hold-ms")].toInt(), 700);

        gestures.touchpad.enabled = false;
        gestures.touchscreen.enabled = false;
        QVERIFY(Ipc::gesturesToJson(gestures).isEmpty());
    }

    void workspaceJsonHasNullNameWhenUnnamed()
    {
        Layout::WorkspaceState state;
        state.index = 2;
        const QJsonObject json = Ipc::workspaceToJson(state);
        QVERIFY(json.value(QStringLiteral("name")).isNull());
        QCOMPARE(json.value(QStringLiteral("idx")).toInt(), 2);
        QVERIFY(json.value(QStringLiteral("active_window_id")).isNull());
    }
};

QTEST_GUILESS_MAIN(TestIpc)

#include "test_ipc.moc"
