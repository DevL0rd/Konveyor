#include "striphelpers.h"

#include <cmath>
#include <limits>

using namespace LayoutTest;

namespace
{

Layout::SizeChange sizeChange(Layout::ChangeKind kind, double value)
{
    return Layout::SizeChange {kind, value};
}

Layout::PositionChange positionChange(Layout::ChangeKind kind, double value)
{
    return Layout::PositionChange {kind, value};
}

}

class TestLayoutFloatingLayer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void positionChangesResolveEveryKind()
    {
        const QRectF area(100, 50, 1000, 500);
        const QPointF current(300, 150);
        using K = Layout::ChangeKind;
        QCOMPARE(
            Layout::applyPositionChange(current, area, positionChange(K::SetFixed, 40), positionChange(K::SetFixed, 10)), QPointF(140, 60));
        QCOMPARE(Layout::applyPositionChange(current, area, positionChange(K::SetProportion, 50), positionChange(K::SetProportion, 20)),
            QPointF(600, 150));
        QCOMPARE(Layout::applyPositionChange(current, area, positionChange(K::AdjustFixed, 25), positionChange(K::AdjustFixed, -5)),
            QPointF(325, 145));
        QCOMPARE(
            Layout::applyPositionChange(current, area, positionChange(K::AdjustProportion, 10), positionChange(K::AdjustProportion, -10)),
            QPointF(400, 100));
    }

    void onlyFixedAndProportionalPositionsAreAbsolute()
    {
        using K = Layout::ChangeKind;
        QVERIFY(Layout::isAbsolutePositionChange(positionChange(K::SetFixed, 1)));
        QVERIFY(Layout::isAbsolutePositionChange(positionChange(K::SetProportion, 1)));
        QVERIFY(!Layout::isAbsolutePositionChange(positionChange(K::AdjustFixed, 1)));
        QVERIFY(!Layout::isAbsolutePositionChange(positionChange(K::AdjustProportion, 1)));
    }

    void sizeChangesResolveEveryKind()
    {
        using K = Layout::ChangeKind;
        FloatKit kit;
        const Layout::WindowId id = kit.add(makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        const auto requested = [&](Layout::SizeChange change, bool horizontal) {
            if (horizontal) {
                kit.layer.setWindowWidth(id, change, false);
            } else {
                kit.layer.setWindowHeight(id, change, false);
            }
            const auto size = kit.layer.tileFor(id)->window().pendingSize();
            return horizontal ? size->width() : size->height();
        };
        QCOMPARE(requested(sizeChange(K::SetFixed, 500), true), 500);
        QCOMPARE(requested(sizeChange(K::AdjustFixed, 50), true), 550);
        QCOMPARE(requested(sizeChange(K::SetProportion, 50), true), 960);
        QVERIFY(requested(sizeChange(K::AdjustProportion, 10), true) > 960);
        QCOMPARE(requested(sizeChange(K::SetFixed, 400), false), 400);
        QCOMPARE(requested(sizeChange(K::AdjustFixed, -100), false), 300);
        QCOMPARE(requested(sizeChange(K::SetProportion, 50), false), 540);
        QVERIFY(requested(sizeChange(K::AdjustProportion, -10), false) < 540);
    }

    void actionsWithoutATargetDoNothing()
    {
        FloatKit kit;
        const Layout::SizeChange change = sizeChange(Layout::ChangeKind::SetFixed, 300);
        kit.layer.setWindowWidth(std::nullopt, change, false);
        kit.layer.setWindowHeight(std::nullopt, change, false);
        kit.layer.toggleWindowWidth(std::nullopt, true, false);
        kit.layer.toggleWindowHeight(std::nullopt, true);
        kit.layer.moveWindow(
            std::nullopt, positionChange(Layout::ChangeKind::SetFixed, 1), positionChange(Layout::ChangeKind::SetFixed, 1), false);
        kit.layer.centerWindow(std::nullopt);
        kit.layer.setFrame(99, QPointF(1, 1), QSizeF(100, 100));
        QVERIFY(kit.layer.isEmpty());
        QVERIFY(!kit.layer.activeWindow());

        const Layout::WindowId id = kit.add();
        const auto before = kit.layer.tileFor(id)->window().pendingSize();
        kit.layer.setWindowWidth(99, change, false);
        kit.layer.setWindowHeight(99, change, false);
        kit.layer.toggleWindowWidth(99, true, false);
        kit.layer.toggleWindowHeight(99, true);
        kit.layer.moveWindow(99, positionChange(Layout::ChangeKind::SetFixed, 1), positionChange(Layout::ChangeKind::SetFixed, 1), false);
        kit.layer.centerWindow(99);
        QCOMPARE(kit.layer.tileFor(id)->window().pendingSize(), before);
    }

    void unknownWindowsAreRejected()
    {
        FloatKit kit;
        kit.add();
        QVERIFY(!kit.layer.activateWindow(99));
        QVERIFY(!kit.layer.animateOpening(99));
        QVERIFY(!kit.layer.updateWindow(99));
        QVERIFY(!kit.layer.childrenAdded(99));
        QVERIFY(!kit.layer.beginResize(99, 1));
        QVERIFY(!kit.layer.tileFor(99));
        QVERIFY(!kit.layer.windowUnder(QPointF(-5, -5)));
    }

    void resizeSessionsAreExclusiveAndWindowBound()
    {
        FloatKit kit;
        const Layout::WindowId first = kit.add();
        const Layout::WindowId second = kit.add();
        const quint8 right = static_cast<quint8>(Layout::ResizeEdge::Right);
        QVERIFY(!kit.layer.updateResize(first, QPointF(10, 0)));
        QVERIFY(kit.layer.beginResize(first, right));
        QVERIFY(!kit.layer.beginResize(second, right));
        QVERIFY(!kit.layer.updateResize(second, QPointF(10, 0)));
        kit.layer.endResize(second);
        QVERIFY(kit.layer.updateResize(first, QPointF(10, 0)));
        kit.layer.endResize(first);
        QVERIFY(!kit.layer.updateResize(first, QPointF(10, 0)));
        kit.layer.endResize(std::nullopt);
        QVERIFY(kit.layer.beginResize(second, right));
    }

    void resizingFromTopLeftGrowsAgainstTheDelta()
    {
        FloatKit kit;
        const Layout::WindowId id = kit.add(makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        kit.layer.tileFor(id)->window().commit(QSizeF(400, 300));
        const quint8 topLeft = static_cast<quint8>(Layout::ResizeEdge::Top) | static_cast<quint8>(Layout::ResizeEdge::Left);
        QVERIFY(kit.layer.beginResize(id, topLeft));
        QVERIFY(kit.layer.updateResize(id, QPointF(-50, -40)));
        QCOMPARE(kit.layer.tileFor(id)->window().pendingSize(), std::optional(QSize(450, 340)));
    }

    void removingATileKeepsItsPendingSizeAndPosition()
    {
        FloatKit kit;
        const Layout::WindowId first = kit.add();
        const Layout::WindowId second = kit.add();
        kit.layer.setWindowWidth(second, sizeChange(Layout::ChangeKind::SetFixed, 640), false);
        const Layout::DetachedTile detached = kit.layer.removeTile(second);
        QCOMPARE(detached.tile.savedFloatingSize, std::optional(QSize(640, 100)));
        QVERIFY(detached.tile.savedFloatingPosition);
        QCOMPARE(kit.layer.activeWindow(), std::optional(first));
        const Layout::DetachedTile plain = kit.layer.removeTile(first);
        QCOMPARE(plain.tile.savedFloatingSize, plain.tile.window().pendingSize());
        QVERIFY(!kit.layer.activeWindow());
    }

    void removingATileWithoutAPendingSizeSavesNone()
    {
        FloatKit kit;
        const Layout::WindowId id = kit.add();
        kit.layer.tileFor(id)->window().requestSize(QSize(100, 100), Layout::WindowMode::Maximized, false);
        const Layout::DetachedTile detached = kit.layer.removeTile(id);
        QVERIFY(!detached.tile.savedFloatingSize);
    }

    void closestPresetNeedsANativeSizeAndPresets()
    {
        FloatKit plain;
        const Layout::WindowId withoutNative = plain.add();
        QVERIFY(!plain.layer.closestWidthPresetIndex(0).has_value());
        QCOMPARE(withoutNative, plain.layer.tiles().front().id());

        Layout::WindowProperties fixed = makeWindow(QStringLiteral("fixed"), {}, QSizeF(960, 540));
        fixed.isResizable = false;
        FloatKit native;
        native.add(fixed);
        QCOMPARE(native.layer.closestWidthPresetIndex(0), std::optional<std::size_t>(1));

        FloatKit bare(optionsWithoutPresets(), QRectF(0, 0, 1920, 1080));
        bare.add(fixed);
        QVERIFY(!bare.layer.closestWidthPresetIndex(0).has_value());
    }

    void removingAResizedTileEndsItsResize()
    {
        FloatKit kit;
        const Layout::WindowId first = kit.add();
        kit.add();
        QVERIFY(kit.layer.beginResize(first, static_cast<quint8>(Layout::ResizeEdge::Right)));
        kit.layer.removeTile(first);
        QVERIFY(kit.layer.beginResize(kit.layer.activeWindow().value(), 1));
    }

    void childWindowsOpenAboveTheirParent()
    {
        FloatKit kit;
        const Layout::WindowId parent = kit.add();
        const Layout::WindowId other = kit.add();
        Layout::WindowProperties child = makeWindow();
        child.parent = other;
        Layout::Tile tile = kit.makeTile(child);
        const Layout::WindowId childId = tile.id();
        kit.layer.insertAbove(parent, std::move(tile), true);
        QCOMPARE(kit.layer.tiles().front().id(), childId);
        QCOMPARE(kit.layer.checkConsistency(), QString());
    }

    void unrelatedWindowsOpenDirectlyAboveTheirSibling()
    {
        FloatKit kit;
        const Layout::WindowId lower = kit.add();
        const Layout::WindowId upper = kit.add();
        Layout::Tile tile = kit.makeTile();
        const Layout::WindowId inserted = tile.id();
        kit.layer.insertAbove(lower, std::move(tile), false);
        QCOMPARE(kit.layer.tiles().at(0).id(), upper);
        QCOMPARE(kit.layer.tiles().at(1).id(), inserted);
        QCOMPARE(kit.layer.tiles().at(2).id(), lower);
        QCOMPARE(kit.layer.activeWindow(), std::optional(upper));
    }

    void windowUnderFindsTheTopmostTile()
    {
        FloatKit kit;
        const Layout::WindowId lower = kit.add(makeWindow(QStringLiteral("a"), {}, QSizeF(400, 300)));
        const QPointF inside = kit.layer.placedTiles(false).front().pos + QPointF(10, 10);
        const Layout::WindowId upper = kit.add(makeWindow(QStringLiteral("b"), {}, QSizeF(400, 300)));
        QCOMPARE(kit.layer.windowUnder(inside), std::optional(upper));
        kit.layer.activateWindow(lower);
        QCOMPARE(kit.layer.windowUnder(inside), std::optional(lower));
    }

    void renderedTilesCanSkipRounding()
    {
        FloatKit kit;
        kit.add();
        QCOMPARE(kit.layer.renderedTilesMut(false).size(), std::size_t(1));
        QCOMPARE(kit.layer.renderedTilesMut(true).size(), std::size_t(1));
    }

    void initialSizeSubtractsBordersOnlyFromTileSizes()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        FloatKit kit(config);
        Layout::EffectiveWindowRules rules;
        const QSize bordered
            = kit.layer.initialWindowSize(Config::PresetSize(Config::Proportion {0.5}), Config::PresetSize(Config::Fixed {300}), rules);
        QCOMPARE(bordered, QSize(952, 300));
        QCOMPARE(kit.layer.initialWindowSize(std::nullopt, std::nullopt, rules), QSize(0, 0));
        FloatKit plain;
        QCOMPARE(plain.layer.initialWindowSize(Config::PresetSize(Config::Proportion {0.5}), std::nullopt, rules), QSize(960, 0));
    }

    void healthyLayerIsConsistent()
    {
        FloatKit kit;
        kit.add();
        kit.add();
        QCOMPARE(kit.layer.checkConsistency(), QString());
        FloatKit empty;
        QCOMPARE(empty.layer.checkConsistency(), QString());
    }

    void rejectsInvalidScale()
    {
        FloatKit kit;
        kit.layer.updateConfig(kit.area.size(), kit.area, 0.0, kit.options);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("scale must be positive and finite")));
        kit.layer.updateConfig(kit.area.size(), kit.area, std::numeric_limits<double>::infinity(), kit.options);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("scale must be positive and finite")));
    }

    void rejectsTilesWithoutData()
    {
        FloatKit kit;
        kit.add();
        kit.layer.tiles().push_back(kit.makeTile());
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("tiles and data must have the same length")));
    }

    void rejectsMissingActiveWindow()
    {
        FloatKit kit;
        kit.add();
        kit.layer.tiles().front() = kit.makeTile();
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("active window must be present in tiles")));
    }

    void rejectsResizeOfMissingWindow()
    {
        FloatKit kit;
        const Layout::WindowId first = kit.add();
        const Layout::WindowId second = kit.add();
        QVERIFY(kit.layer.beginResize(first, static_cast<quint8>(Layout::ResizeEdge::Right)));
        std::erase_if(kit.layer.tiles(), [first](const Layout::Tile &tile) { return tile.id() == first; });
        kit.layer.tiles().push_back(kit.makeTile());
        QCOMPARE(kit.layer.activeWindow(), std::optional(second));
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("interactive resize window must be present in tiles")));
    }

    void rejectsMaximizedWindows()
    {
        FloatKit kit;
        const Layout::WindowId id = kit.add();
        kit.layer.tileFor(id)->window().requestSize(QSize(100, 100), Layout::WindowMode::Maximized, false);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("cannot be maximized or fullscreen")));
    }

    void rejectsExpansionExemptions()
    {
        FloatKit kit;
        const Layout::WindowId id = kit.add();
        kit.layer.tileFor(id)->window().setExpansionForceResizable(true);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("keeps the size exemption of an expanded column")));
    }

    void rejectsWindowsOutsideTheirLimits()
    {
        FloatKit kit;
        const Layout::WindowId id = kit.add();
        Layout::WindowProperties tooBig = makeWindow();
        tooBig.minSize = QSizeF(5000, 100);
        kit.layer.tileFor(id)->window().setProperties(tooBig);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("outside its minimum or maximum size")));
        Layout::WindowProperties tooSmall = makeWindow();
        tooSmall.maxSize = QSizeF(50, 50);
        kit.layer.tileFor(id)->window().setProperties(tooSmall);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("outside its minimum or maximum size")));
        Layout::WindowProperties tooShort = makeWindow();
        tooShort.minSize = QSizeF(0, 5000);
        kit.layer.tileFor(id)->window().setProperties(tooShort);
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("outside its minimum or maximum size")));
    }

    void rejectsStaleTileData()
    {
        FloatKit kit;
        const Layout::WindowId id = kit.add();
        kit.layer.tileFor(id)->window().commit(QSizeF(700, 500));
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("tile data must be up to date")));
    }

    void rejectsChildrenBelowTheirParents()
    {
        FloatKit kit;
        const Layout::WindowId parent = kit.add();
        Layout::WindowProperties child = makeWindow();
        child.parent = parent;
        kit.add(child);
        QCOMPARE(kit.layer.checkConsistency(), QString());
        std::swap(kit.layer.tiles().at(0), kit.layer.tiles().at(1));
        QVERIFY(kit.layer.checkConsistency().contains(QStringLiteral("children must be stacked above parents")));
    }

    void raisingAParentKeepsNestedChildrenAbove()
    {
        FloatKit kit;
        const Layout::WindowId root = kit.add();
        Layout::WindowProperties childProperties = makeWindow();
        childProperties.parent = root;
        const Layout::WindowId child = kit.add(childProperties);
        Layout::WindowProperties grandProperties = makeWindow();
        grandProperties.parent = child;
        const Layout::WindowId grandchild = kit.add(grandProperties);
        const Layout::WindowId unrelated = kit.add();
        kit.layer.activateWindow(root);
        const auto indexOf = [&](Layout::WindowId id) {
            const auto &tiles = kit.layer.tiles();
            return std::find_if(tiles.begin(), tiles.end(), [id](const Layout::Tile &tile) { return tile.id() == id; }) - tiles.begin();
        };
        QVERIFY(indexOf(grandchild) < indexOf(child));
        QVERIFY(indexOf(child) < indexOf(root));
        QVERIFY(indexOf(root) < indexOf(unrelated));
        QCOMPARE(kit.layer.checkConsistency(), QString());
    }
};

QTEST_GUILESS_MAIN(TestLayoutFloatingLayer)
#include "test_layout_floatinglayer.moc"
