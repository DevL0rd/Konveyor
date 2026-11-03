#include "config/loader.h"
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
        const QJsonObject json
            = Ipc::windowToJson({3, QStringLiteral("Title"), QStringLiteral("app"), 1234, QStringLiteral("{0f}")}, state);
        QCOMPARE(json.value(QStringLiteral("app_id")).toString(), QStringLiteral("app"));
        QCOMPARE(json.value(QStringLiteral("uuid")).toString(), QStringLiteral("{0f}"));
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

    void gesturesJsonLeavesOutWindowSwipesTheRowSwipesTakeOver_data()
    {
        QTest::addColumn<bool>("horizontalOff");
        QTest::addColumn<bool>("verticalOff");
        QTest::addColumn<QStringList>("motions");
        QTest::newRow("row swipes on") << false << false
                                       << QStringList {QStringLiteral("swipe-horizontal"), QStringLiteral("swipe-vertical")};
        QTest::newRow("row swipes off") << true << true
                                        << QStringList {QStringLiteral("window-swipe-horizontal"), QStringLiteral("window-swipe-vertical")};
        QTest::newRow("only the vertical row swipe") << true << false << QStringList {QStringLiteral("swipe-vertical")};
    }

    void gesturesJsonLeavesOutWindowSwipesTheRowSwipesTakeOver()
    {
        QFETCH(bool, horizontalOff);
        QFETCH(bool, verticalOff);
        QFETCH(QStringList, motions);
        Config::Gestures gestures;
        gestures.touchscreen.enabled = false;
        gestures.touchpad.windowSwipeFingers = gestures.touchpad.swipeFingers;
        gestures.touchpad.pinch = Config::PinchAction::Off;
        gestures.touchpad.threeFingerTap = Config::TapAction::Off;
        gestures.touchpad.fourFingerTap = Config::TapAction::Off;
        if (horizontalOff) {
            gestures.touchpad.horizontalSwipe = Config::HorizontalSwipe::Off;
        }
        if (verticalOff) {
            gestures.touchpad.verticalSwipe = Config::VerticalSwipe::Off;
        }
        QStringList listed;
        for (const QJsonValue &gesture : Ipc::gesturesToJson(gestures)) {
            listed.append(gesture.toObject()[QStringLiteral("motion")].toString());
        }
        QCOMPARE(listed, motions);
    }

    void longPressIsListedOnlyWhenTheTouchscreenMovesWindowsThatWay()
    {
        Config::Gestures gestures;
        gestures.touchpad.enabled = false;
        gestures.touchscreen.horizontalSwipe = Config::HorizontalSwipe::Off;
        gestures.touchscreen.verticalSwipe = Config::VerticalSwipe::Off;
        gestures.touchscreen.windowHorizontalSwipe = Config::WindowHorizontalSwipe::Off;
        gestures.touchscreen.windowVerticalSwipe = Config::WindowVerticalSwipe::Off;
        gestures.touchscreen.pinch = Config::PinchAction::Off;
        gestures.touchscreen.threeFingerTap = Config::TapAction::Off;
        gestures.touchscreen.fourFingerTap = Config::TapAction::Off;
        gestures.touchscreen.fiveFingerTap = Config::TapAction::Off;
        gestures.touchscreen.longPressToMove = true;
        QCOMPARE(Ipc::gesturesToJson(gestures).size(), 1);
        gestures.touchscreen.longPressToMove = false;
        QVERIFY(Ipc::gesturesToJson(gestures).isEmpty());
    }

    void outputJsonDescribesTheLogicalGeometry()
    {
        Layout::OutputInfo info;
        info.name = QStringLiteral("DP-1");
        info.makeModelSerial = QStringLiteral("Acme Panel 42");
        info.geometry = QRectF(10, 20, 1920, 1080);
        info.scale = 1.5;
        const QJsonObject json = Ipc::outputToJson(info);
        QCOMPARE(json.value(QStringLiteral("name")).toString(), QStringLiteral("DP-1"));
        QCOMPARE(json.value(QStringLiteral("description")).toString(), QStringLiteral("Acme Panel 42"));
        const QJsonObject logical = json.value(QStringLiteral("logical")).toObject();
        QCOMPARE(logical.value(QStringLiteral("x")).toDouble(), 10.0);
        QCOMPARE(logical.value(QStringLiteral("y")).toDouble(), 20.0);
        QCOMPARE(logical.value(QStringLiteral("width")).toDouble(), 1920.0);
        QCOMPARE(logical.value(QStringLiteral("height")).toDouble(), 1080.0);
        QCOMPARE(logical.value(QStringLiteral("scale")).toDouble(), 1.5);
    }

    void bindJsonCarriesTheKeyActionAndOverlayTitle()
    {
        const auto loaded = Config::loadString(QStringLiteral(R"(
            binds {
                Mod+Q hotkey-overlay-title="Quit it" { close-window; }
                Mod+W hotkey-overlay-title=null { close-window; }
                Mod+E { close-window; }
            }
        )"),
            QStringLiteral("config.kdl"));
        QVERIFY(loaded.has_value());
        const QList<Config::Bind> &binds = loaded->config.binds;
        QCOMPARE(binds.size(), 3);
        const QJsonObject titled = Ipc::bindToJson(binds.at(0));
        QCOMPARE(titled.value(QStringLiteral("key")).toString(), Config::bindKeyLabel(binds.at(0)));
        QCOMPARE(
            titled.value(QStringLiteral("action")).toObject().value(QStringLiteral("name")).toString(), QStringLiteral("close-window"));
        QCOMPARE(titled.value(QStringLiteral("title")).toString(), QStringLiteral("Quit it"));
        QCOMPARE(titled.value(QStringLiteral("hidden")).toBool(), false);
        const QJsonObject hidden = Ipc::bindToJson(binds.at(1));
        QVERIFY(hidden.value(QStringLiteral("title")).isNull());
        QCOMPARE(hidden.value(QStringLiteral("hidden")).toBool(), true);
        QVERIFY(Ipc::bindToJson(binds.at(2)).value(QStringLiteral("title")).isNull());
    }

    void workspaceJsonCarriesAName()
    {
        Layout::WorkspaceState state;
        state.name = QStringLiteral("chat");
        QCOMPARE(Ipc::workspaceToJson(state).value(QStringLiteral("name")).toString(), QStringLiteral("chat"));
    }

    void actionFromJsonAcceptsNumbersAndBooleansAndMissingIds()
    {
        const QJsonObject object {{QStringLiteral("name"), QStringLiteral("focus-workspace")},
            {QStringLiteral("arguments"), QJsonArray {2, QStringLiteral("up")}},
            {QStringLiteral("properties"), QJsonObject {{QStringLiteral("focus"), true}, {QStringLiteral("label"), QStringLiteral("x")}}}};
        const auto request = Ipc::actionFromJson(object);
        QVERIFY(request.has_value());
        QCOMPARE(request->action.arguments, (QStringList {QStringLiteral("2"), QStringLiteral("up")}));
        QCOMPARE(request->action.properties.size(), 2);
        QCOMPARE(request->action.properties.at(0).first, QStringLiteral("focus"));
        QCOMPARE(request->action.properties.at(0).second, QStringLiteral("true"));
        QCOMPARE(request->action.properties.at(1).second, QStringLiteral("x"));
        QVERIFY(!request->target.has_value());
    }

    void windowIdFlagHasAnAlias()
    {
        const auto request = Ipc::actionFromArguments({QStringLiteral("close-window"), QStringLiteral("--window-id"), QStringLiteral("9")});
        QVERIFY(request.has_value());
        QCOMPARE(request->target.value(), Layout::WindowId {9});
    }

    void workspaceJsonHasNullNameWhenUnnamed()
    {
        Layout::WorkspaceState state;
        state.index = 2;
        const QJsonObject json = Ipc::workspaceToJson(state);
        QVERIFY(json.value(QStringLiteral("name")).isNull());
        QCOMPARE(json.value(QStringLiteral("idx")).toInt(), 2);
        QVERIFY(json.value(QStringLiteral("active_window_id")).isNull());
        QCOMPARE(json.value(QStringLiteral("columns")).toArray(), QJsonArray());
        QCOMPARE(json.value(QStringLiteral("group_app_windows")).toString(), QStringLiteral("beside"));
    }

    void workspaceJsonListsColumnsInOrder()
    {
        Layout::WorkspaceState state;
        state.columns = {{4}, {7, 2}};
        state.columnDisplays = {Config::ColumnDisplay::Normal, Config::ColumnDisplay::Tabbed};
        state.groupAppWindows = Config::GroupAppWindows::Stack;
        const QJsonObject json = Ipc::workspaceToJson(state);
        QCOMPARE(json.value(QStringLiteral("columns")).toArray(), (QJsonArray {QJsonArray {4}, QJsonArray {7, 2}}));
        QCOMPARE(
            json.value(QStringLiteral("column_displays")).toArray(), (QJsonArray {QStringLiteral("normal"), QStringLiteral("tabbed")}));
        QCOMPARE(json.value(QStringLiteral("group_app_windows")).toString(), QStringLiteral("stack"));
    }
};

QTEST_GUILESS_MAIN(TestIpc)

#include "test_ipc.moc"
