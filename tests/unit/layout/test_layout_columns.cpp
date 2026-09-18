#include "helpers.h"

using namespace LayoutTest;

class TestLayoutColumns : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void columnsNeverOverlapAfterWidthChanges()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        const auto third = fixture.add();
        const QList<Layout::WindowId> ids {first, second, third};
        const QStringList widths {QStringLiteral("30%"), QStringLiteral("70%"), QStringLiteral("45%")};
        for (qsizetype i = 0; i < ids.size(); ++i) {
            fixture.engine().activateWindow(ids.at(i));
            fixture.perform(QStringLiteral("set-column-width"), {widths.at(i)});
            for (qsizetype earlier = 0; earlier < ids.size() - 1; ++earlier) {
                const QRectF left = fixture.frame(ids.at(earlier));
                const QRectF right = fixture.frame(ids.at(earlier + 1));
                QVERIFY2(left.right() <= right.left() + 0.001, "columns overlap after a width change");
            }
        }
        VERIFY_INVARIANTS(fixture);
    }

    void singleWindowUsesDefaultColumnWidth()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id), QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void secondWindowOpensAsNewColumn()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 1);
        QCOMPARE(fixture.focused(), second);
        VERIFY_INVARIANTS(fixture);
    }

    void keepsWindowSizeWhenDefaultWidthIsUnset()
    {
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth = std::nullopt;
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        QCOMPARE(fixture.frame(id).width(), 400.0);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedDefaultColumnWidthIsHonored()
    {
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {500});
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).width(), 500.0);
        VERIFY_INVARIANTS(fixture);
    }

    void tiledSizeRulesOverrideApplicationHints()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("fixed"));
        rule.forceResizable = true;
        rule.minWidth = 600;
        rule.maxWidth = 800;
        config.windowRules.append(rule);
        Fixture fixture(config);
        Layout::WindowProperties properties = makeWindow(QStringLiteral("fixed"));
        properties.minSize = QSizeF(300, 200);
        properties.maxSize = QSizeF(300, 200);
        const auto id = fixture.addWith(properties);
        QCOMPARE(fixture.frame(id).width(), 800.0);
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("500")}).ok);
        QCOMPARE(fixture.frame(id).width(), 600.0);
        VERIFY_INVARIANTS(fixture);
    }

    void setColumnWidthProportion()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("25%")}).ok);
        QCOMPARE(fixture.frame(id).width(), (1920.0 - 16.0) * 0.25 - 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void setColumnWidthFixedAndAdjust()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("400")}).ok);
        QCOMPARE(fixture.frame(id).width(), 400.0);
        QVERIFY(fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("+50")}).ok);
        QCOMPARE(fixture.frame(id).width(), 450.0);
        VERIFY_INVARIANTS(fixture);
    }

    void switchPresetColumnWidthCycles()
    {
        Config::Config config = instantConfig();
        config.layout.presetColumnWidths = {Config::PresetSize(Config::Fixed {300}), Config::PresetSize(Config::Fixed {600})};
        Fixture fixture(config);
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        const double first = fixture.frame(id).width();
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        const double second = fixture.frame(id).width();
        QVERIFY(first != second);
        fixture.perform(QStringLiteral("switch-preset-column-width-back"));
        QCOMPARE(fixture.frame(id).width(), first);
        VERIFY_INVARIANTS(fixture);
    }

    void consumeAndExpelWindow()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(second).tileIndex, 1);
        VERIFY_INVARIANTS(fixture);

        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-right")).ok);
        QCOMPARE(fixture.state(second).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void twoWindowsInColumnShareHeight()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        const double total = fixture.frame(first).height();
        QCOMPARE(total, (1080.0 - 16.0 * 3) / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void setWindowHeightFixed()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(
            fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}, {{QStringLiteral("id"), QString::number(first)}})
                .ok);
        QCOMPARE(fixture.frame(first).height(), 300.0);
        VERIFY_INVARIANTS(fixture);
    }

    void resetWindowHeightRestoresAuto()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.add();
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}, {{QStringLiteral("id"), QString::number(first)}});
        fixture.perform(QStringLiteral("reset-window-height"), {}, {{QStringLiteral("id"), QString::number(first)}});
        QCOMPARE(fixture.frame(first).height(), (1080.0 - 16.0 * 3) / 2.0);
        VERIFY_INVARIANTS(fixture);
    }

    void moveColumnLeftAndRight()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-column-left")).ok);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 1);
        QVERIFY(fixture.perform(QStringLiteral("move-column-right")).ok);
        QCOMPARE(fixture.state(second).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void moveColumnToIndexIsOneBased()
    {
        Fixture fixture;
        fixture.add();
        fixture.add();
        const auto third = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-index"), {QStringLiteral("1")}).ok);
        QCOMPARE(fixture.state(third).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void focusColumnByIndex()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("1")}).ok);
        QCOMPARE(fixture.focused(), first);
        VERIFY_INVARIANTS(fixture);
    }

    void minWidthRuleIsApplied()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule;
        rule.minWidth = 1200;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.frame(id).width(), 1200.0);
        VERIFY_INVARIANTS(fixture);
    }

    void swapWindowsBetweenColumns()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("swap-window-left")).ok);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void expandColumnToAvailableWidth()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("300")});
        QVERIFY(fixture.perform(QStringLiteral("expand-column-to-available-width")).ok);
        QVERIFY(fixture.frame(id).width() > 300.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutColumns)
#include "test_layout_columns.moc"
