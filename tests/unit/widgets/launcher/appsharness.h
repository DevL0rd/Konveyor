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

    QStringList tiles(const QString &property)
    {
        return eval(QStringLiteral("(g => { const out = []; for (let i = 0; i < g.count; ++i) { const item = g.itemAtIndex(i); "
                                   "out.push(item ? String(item.%1) : '') } return out })(%2)")
                        .arg(property, m_grid))
            .toStringList();
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

    QQuickItem *tile(int position)
    {
        return qvariant_cast<QQuickItem *>(eval(QStringLiteral("%1.itemAtIndex(%2)").arg(m_grid).arg(position)));
    }

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
