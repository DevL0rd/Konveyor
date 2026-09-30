#pragma once

#include "launcherharness.h"

namespace AppsTest
{

inline const QStringList everyApp
    = {QStringLiteral("Ark"), QStringLiteral("Asteroid Blaster"), QStringLiteral("Blender"), QStringLiteral("Chess Master"),
        QStringLiteral("Discord"), QStringLiteral("Dolphin"), QStringLiteral("Elisa"), QStringLiteral("Firefox"),
        QStringLiteral("GNU Image Manipulation Program"), QStringLiteral("Heroic Games Launcher"), QStringLiteral("Kate"),
        QStringLiteral("KCalc"), QStringLiteral("Konsole"), QStringLiteral("Krita"), QStringLiteral("LibreOffice Writer"),
        QStringLiteral("Mystery Tool"), QStringLiteral("Obsidian"), QStringLiteral("Steam"), QStringLiteral("Zed")};

inline const QList<QPair<QString, QStringList>> &categories()
{
    static const QList<QPair<QString, QStringList>> list = {
        {QStringLiteral("All Applications"), everyApp},
        {QStringLiteral("Development"), {QStringLiteral("Kate"), QStringLiteral("Zed")}},
        {QStringLiteral("Games"),
            {QStringLiteral("Asteroid Blaster"), QStringLiteral("Chess Master"), QStringLiteral("Heroic Games Launcher"),
                QStringLiteral("Steam")}},
        {QStringLiteral("Graphics"),
            {QStringLiteral("Blender"), QStringLiteral("GNU Image Manipulation Program"), QStringLiteral("Krita")}},
        {QStringLiteral("Internet"), {QStringLiteral("Discord"), QStringLiteral("Firefox"), QStringLiteral("Steam")}},
        {QStringLiteral("Lost & Found"), {QStringLiteral("Mystery Tool")}},
        {QStringLiteral("Multimedia"), {QStringLiteral("Elisa")}},
        {QStringLiteral("Office"), {QStringLiteral("LibreOffice Writer"), QStringLiteral("Obsidian")}},
        {QStringLiteral("System"), {QStringLiteral("Dolphin"), QStringLiteral("Konsole")}},
        {QStringLiteral("Utilities"), {QStringLiteral("Ark"), QStringLiteral("Kate"), QStringLiteral("KCalc")}},
    };
    return list;
}

inline QString idOf(const QString &label)
{
    static const QHash<QString, QString> ids = {{QStringLiteral("Ark"), QStringLiteral("org.kde.ark")},
        {QStringLiteral("Asteroid Blaster"), QStringLiteral("org.example.blaster")},
        {QStringLiteral("Blender"), QStringLiteral("org.blender.Blender")},
        {QStringLiteral("Chess Master"), QStringLiteral("org.example.chess")},
        {QStringLiteral("Discord"), QStringLiteral("com.discordapp.Discord")},
        {QStringLiteral("Dolphin"), QStringLiteral("org.kde.dolphin")}, {QStringLiteral("Elisa"), QStringLiteral("org.kde.elisa")},
        {QStringLiteral("Firefox"), QStringLiteral("org.mozilla.firefox")},
        {QStringLiteral("GNU Image Manipulation Program"), QStringLiteral("org.gimp.GIMP")},
        {QStringLiteral("Heroic Games Launcher"), QStringLiteral("com.heroicgameslauncher.hgl")},
        {QStringLiteral("Kate"), QStringLiteral("org.kde.kate")}, {QStringLiteral("KCalc"), QStringLiteral("org.kde.kcalc")},
        {QStringLiteral("Konsole"), QStringLiteral("org.kde.konsole")}, {QStringLiteral("Krita"), QStringLiteral("org.kde.krita")},
        {QStringLiteral("LibreOffice Writer"), QStringLiteral("org.libreoffice.Writer")},
        {QStringLiteral("Mystery Tool"), QStringLiteral("org.example.mystery")},
        {QStringLiteral("Obsidian"), QStringLiteral("md.obsidian.Obsidian")},
        {QStringLiteral("Steam"), QStringLiteral("com.valvesoftware.Steam")}, {QStringLiteral("Zed"), QStringLiteral("dev.zed.Zed")},
        {QStringLiteral("Brand New IDE"), QStringLiteral("org.example.newide")}};
    return ids.value(label);
}

inline QStringList firstThen(const QStringList &first, const QStringList &all)
{
    QStringList ranked;
    QStringList rest = all;
    for (const QString &label : first) {
        if (rest.removeAll(label) > 0) {
            ranked.append(label);
        }
    }
    return ranked + rest;
}

class TestCase : public LauncherTest::TestCase
{
protected:
    const QString m_grid = QStringLiteral("launcher.currentView().sections[0]");

    QVariant grid(const QString &expression) { return eval(m_grid + QLatin1Char('.') + expression); }

    QStringList tilesMapped(const QString &grid, const QString &mapping)
    {
        return eval(QStringLiteral("(g => { const out = []; for (let i = 0; i < g.shownCount; ++i) { const item = g.itemAtIndex(i); "
                                   "out.push(item ? String((%1)(item)) : '') } return out })(%2)")
                        .arg(mapping, grid))
            .toStringList();
    }

    QStringList tilesOf(const QString &grid, const QString &property)
    {
        return tilesMapped(grid, QStringLiteral("item => item.") + property);
    }

    QString staleTiles(const QString &grid)
    {
        const QStringList shown = tilesMapped(grid, QStringLiteral("t => [t.label, t.iconSource, t.favoriteId]"));
        const QStringList current = tilesMapped(grid,
            QStringLiteral("t => t.model ? [t.model.display || '', t.model.decoration, t.model.favoriteId || ''] : [t.label, t.iconSource, "
                           "t.favoriteId]"));
        QStringList stale;
        for (qsizetype i = 0; i < shown.size(); ++i) {
            if (shown.at(i) != current.value(i)) {
                stale.append(QStringLiteral("%1 shows %2").arg(current.value(i), shown.at(i)));
            }
        }
        return stale.join(QStringLiteral(", "));
    }

    QStringList tiles(const QString &property) { return tilesOf(m_grid, property); }

    QQuickItem *tileOf(const QString &grid, int position)
    {
        return qvariant_cast<QQuickItem *>(eval(QStringLiteral("%1.itemAtIndex(%2)").arg(grid).arg(position)));
    }

    QList<int> selectedIn(const QString &grid)
    {
        const QStringList flags = tilesOf(grid, QStringLiteral("selected"));
        QList<int> selected;
        for (qsizetype i = 0; i < flags.size(); ++i) {
            if (flags.at(i) == QLatin1String("true")) {
                selected.append(int(i));
            }
        }
        return selected;
    }

    static QQuickItem *clickTarget(QQuickItem *item, const QPointF &scene, const QQuickItem *content)
    {
        if (!item->isVisible() || !item->isEnabled()) {
            return nullptr;
        }
        QList<QQuickItem *> children = item->childItems();
        std::stable_sort(children.begin(), children.end(), [](QQuickItem *a, QQuickItem *b) { return a->z() < b->z(); });
        for (auto it = children.crbegin(); it != children.crend(); ++it) {
            if (QQuickItem *found = clickTarget(*it, scene, content)) {
                return found;
            }
        }
        const bool clickable = item->inherits("QQuickMouseArea") || item->inherits("QQuickAbstractButton")
            || ((item->acceptedMouseButtons() & Qt::LeftButton) && content->isAncestorOf(item));
        return clickable && item->contains(item->mapFromScene(scene)) ? item : nullptr;
    }

    QString hitsOtherTiles(const QString &grid)
    {
        QStringList wrong;
        const auto *view = qvariant_cast<QQuickItem *>(eval(grid));
        const auto *content = view ? view->property("contentItem").value<QQuickItem *>() : nullptr;
        const int shown = eval(grid + QStringLiteral(".shownCount")).toInt();
        for (int position = 0; position < shown; ++position) {
            QQuickItem *expected = tileOf(grid, position);
            if (!expected || !content) {
                wrong.append(QStringLiteral("no tile at %1").arg(position));
                continue;
            }
            QQuickItem *hit = clickTarget(
                m_harness.window()->contentItem(), expected->mapToScene(QPointF(expected->width() / 2, expected->height() / 2)), content);
            while (hit && hit->parentItem() != content) {
                hit = hit->parentItem();
            }
            if (hit != expected) {
                wrong.append(QStringLiteral("%1 under %2")
                        .arg(hit ? hit->property("label").toString() : QStringLiteral("nothing"), expected->property("label").toString()));
            }
        }
        return wrong.join(QStringLiteral(", "));
    }

    bool hoverLightsOnlyThat(const QString &grid, int position)
    {
        QQuickItem *item = tileOf(grid, position);
        if (!item) {
            return false;
        }
        const bool flagged = item->property("selected").isValid();
        hover(item);
        return QTest::qWaitFor(
            [this, &grid, position, flagged] {
                return eval(grid + QStringLiteral(".currentIndex")).toInt() == position
                    && eval(grid + QStringLiteral(".sectionActive")).toBool() && (!flagged || selectedIn(grid) == QList<int> {position});
            },
            30000);
    }

    QStringList shownLabels() { return tiles(QStringLiteral("label")); }

    QList<QQuickItem *> chips()
    {
        QList<QQuickItem *> found;
        QList<QQuickItem *> all = {m_harness.window()->contentItem()};
        for (qsizetype i = 0; i < all.size(); ++i) {
            QQuickItem *item = all.at(i);
            all.append(item->childItems());
            if (!QString::fromLatin1(item->metaObject()->className()).startsWith(QLatin1String("Segment_")) || !item->isVisible()) {
                continue;
            }
            QQuickItem *ancestor = item->parentItem();
            for (int depth = 0; ancestor && depth < 4; ++depth, ancestor = ancestor->parentItem()) {
                if (QString::fromLatin1(ancestor->metaObject()->className()) == QLatin1String("QQuickFlickable")) {
                    found.append(item);
                    break;
                }
            }
        }
        std::sort(found.begin(), found.end(), [](QQuickItem *a, QQuickItem *b) { return a->x() < b->x(); });
        return found;
    }

    QStringList chipLabels()
    {
        QStringList labels;
        for (QQuickItem *chip : chips()) {
            labels.append(chip->property("text").toString());
        }
        return labels;
    }

    void click(QQuickItem *item, Qt::MouseButton button = Qt::LeftButton)
    {
        QTest::mouseClick(m_harness.window(), button, {}, item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
    }

    void hover(QQuickItem *item)
    {
        const QPoint target = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
        QTest::mouseMove(m_harness.window(), QPoint(1, 1));
        QTest::mouseMove(m_harness.window(), target);
    }

    QQuickItem *tile(int position) { return tileOf(m_grid, position); }

    bool showCategory(const QString &name)
    {
        for (QQuickItem *chip : chips()) {
            if (chip->property("text").toString() == name) {
                click(chip);
                return QTest::qWaitFor([chip] { return chip->property("current").toBool(); }, 30000);
            }
        }
        return false;
    }

    bool waitForChips()
    {
        return QTest::qWaitFor([this] { return chips().size() == categories().size(); }, 30000);
    }

    bool openApps(const QVariantMap &settings = {})
    {
        return m_harness.openHost(false, settings) && goTo(QStringLiteral("apps")) && waitForChips();
    }

    bool reopenApps()
    {
        m_harness.root()->setProperty("open", true);
        return QTest::qWaitFor([this] { return m_harness.view()->property("progress").toDouble() == 1.0; }, 30000)
            && goTo(QStringLiteral("apps")) && waitForChips();
    }

    QStringList launchedAfterClicking(int position)
    {
        QQuickItem *item = tile(position);
        if (!item) {
            return {QStringLiteral("no tile at %1").arg(position)};
        }
        const qsizetype before = m_harness.launched().size();
        click(item);
        if (!QTest::qWaitFor([this, before] { return m_harness.launched().size() > before; }, 30000)) {
            return {QStringLiteral("nothing launched from %1").arg(item->property("label").toString())};
        }
        return m_harness.launched().mid(before);
    }

    void setSort(const QString &sort) { m_harness.config()->insert(QStringLiteral("appsSort"), sort); }
};

}
