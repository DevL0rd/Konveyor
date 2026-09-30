#include "helpers.h"

#include <QJsonDocument>

using namespace LayoutTest;

namespace
{

Config::Config memoryConfig(bool positions)
{
    Config::Config config = instantConfig();
    config.layout.rememberWindowSizes = true;
    config.layout.rememberWindowPositions = positions;
    return config;
}

Layout::WindowProperties dialog(const QString &appId)
{
    Layout::WindowProperties properties = makeWindow(appId, appId, QSizeF(400, 300));
    properties.isDialog = true;
    return properties;
}

}

class TestLayoutMemory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void reopensAtRememberedColumnWidth()
    {
        int changes = 0;
        Layout::Hooks hooks;
        hooks.windowMemoryChanged = [&changes] { ++changes; };
        Fixture fixture(memoryConfig(false), QRectF(0, 0, 1920, 1080), hooks);
        const auto first = fixture.add(QStringLiteral("editor"));
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("70%")});
        const double width = fixture.frame(first).width();
        fixture.remove(first);
        const auto reopened = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.frame(reopened).width(), width);
        QVERIFY(changes > 0);
        const auto other = fixture.add(QStringLiteral("other"));
        QCOMPARE(fixture.frame(other).width(), 936.0);
    }

    void windowRuleWidthWinsOverMemory()
    {
        Config::Config config = memoryConfig(false);
        Config::WindowRule rule;
        Config::Match match;
        match.appId = QRegularExpression(QStringLiteral("^editor$"));
        rule.matches.append(match);
        rule.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Proportion {1.0});
        config.windowRules.append(rule);
        Fixture fixture(config);
        Layout::WindowMemory memory;
        memory[QStringLiteral("editor")].columnWidth = Layout::ColumnWidth::proportion(0.25);
        fixture.engine().setWindowMemory(memory);
        const auto id = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.frame(id).width(), 1888.0);
    }

    void memoryOffKeepsDefaults()
    {
        Fixture fixture;
        Layout::WindowMemory memory;
        memory[QStringLiteral("editor")].columnWidth = Layout::ColumnWidth::proportion(0.25);
        fixture.engine().setWindowMemory(memory);
        const auto id = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.frame(id).width(), 936.0);
        QCOMPARE(fixture.engine().windowMemory(), memory);
    }

    void nativeSizeIsAlwaysRememberedAndRestored()
    {
        int changes = 0;
        Layout::Hooks hooks;
        hooks.windowMemoryChanged = [&changes] { ++changes; };
        Fixture fixture(instantConfig(), QRectF(0, 0, 1920, 1080), hooks);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("game"), QStringLiteral("game"), QSizeF(300, 200));
        properties.isResizable = false;
        const auto first = fixture.addWith(properties);
        fixture.remove(first);

        Layout::WindowMemory memory = fixture.engine().windowMemory();
        QCOMPARE(memory[QStringLiteral("game")].nativeSize, std::optional(QSize(300, 200)));
        QVERIFY(changes > 0);

        properties.frameSize = QSizeF(800, 600);
        const auto reopened = fixture.addWith(properties);
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(300, 200));
    }

    void reopensFloatingAtRememberedSizeAndPosition()
    {
        Fixture fixture(memoryConfig(true));
        const auto first = fixture.addWith(dialog(QStringLiteral("picker")));
        QVERIFY(fixture.state(first).isFloating);
        fixture.engine().setFloatingFrame(first, QRectF(300, 200, 640, 480));
        fixture.settle();
        const QRectF before = fixture.frame(first);
        fixture.remove(first);
        const auto reopened = fixture.addWith(dialog(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(reopened), before);
    }

    void floatingPositionIsNotRememberedByDefault()
    {
        Fixture fixture(memoryConfig(false));
        const auto first = fixture.addWith(dialog(QStringLiteral("picker")));
        fixture.engine().setFloatingFrame(first, QRectF(300, 200, 640, 480));
        fixture.settle();
        fixture.remove(first);
        QVERIFY(!fixture.engine().windowMemory().value(QStringLiteral("picker")).floatingPosition);
        const auto reopened = fixture.addWith(dialog(QStringLiteral("picker")));
        QCOMPARE(fixture.frame(reopened).size(), QSizeF(640, 480));
    }

    void memoryRoundTripsThroughJson()
    {
        Layout::WindowMemory memory;
        memory[QStringLiteral("editor")].columnWidth = Layout::ColumnWidth::fixed(812);
        memory[QStringLiteral("picker")].floatingSize = QSize(640, 480);
        memory[QStringLiteral("picker")].floatingPosition = QPointF(0.25, 0.5);
        memory[QStringLiteral("browser")].columnWidth = Layout::ColumnWidth::proportion(0.5);
        memory[QStringLiteral("game")].nativeSize = QSize(1280, 720);
        QCOMPARE(Layout::windowMemoryFromJson(Layout::windowMemoryToJson(memory)), memory);
    }

    void damagedMemoryDoesNotOpenWindowsAtAnUnusableSize_data()
    {
        QTest::addColumn<QByteArray>("entry");
        QTest::newRow("zero proportion") << QByteArray(R"({"column-width": {"proportion": true, "value": 0}})");
        QTest::newRow("negative proportion") << QByteArray(R"({"column-width": {"proportion": true, "value": -2}})");
        QTest::newRow("huge proportion") << QByteArray(R"({"column-width": {"proportion": true, "value": 1e9}})");
        QTest::newRow("zero width") << QByteArray(R"({"column-width": {"proportion": false, "value": 0}})");
        QTest::newRow("huge width") << QByteArray(R"({"column-width": {"proportion": false, "value": 1e12}})");
        QTest::newRow("width as text") << QByteArray(R"({"column-width": {"proportion": false, "value": "wide"}})");
        QTest::newRow("negative floating size") << QByteArray(R"({"floating-size": [-10, -10]})");
        QTest::newRow("empty floating size") << QByteArray(R"({"floating-size": [0, 0]})");
        QTest::newRow("floating size as text") << QByteArray(R"({"floating-size": ["a", "b"]})");
        QTest::newRow("floating position as text") << QByteArray(R"({"floating-position": ["a", "b"]})");
    }

    void damagedMemoryDoesNotOpenWindowsAtAnUnusableSize()
    {
        QFETCH(QByteArray, entry);
        Fixture fixture(memoryConfig(true));
        fixture.add(QStringLiteral("other"));
        const QJsonObject json {{QStringLiteral("app"), QJsonDocument::fromJson(entry).object()}};
        fixture.engine().setWindowMemory(Layout::windowMemoryFromJson(json));
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("app")), Layout::RememberedWindow());
        const auto tiled = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.frame(tiled).width(), 936.0);
        const auto floating = fixture.addWith(dialog(QStringLiteral("app")));
        QCOMPARE(fixture.frame(floating), QRectF(760, 390, 400, 300));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMemory)
#include "test_layout_memory.moc"
