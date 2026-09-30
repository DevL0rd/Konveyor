#include "portalharness.h"
#include "qmlsearch.h"

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QQuickWindow>
#include <QTest>
#include <QtQuickTest/quicktest.h>

using Konveyor::Test::PortalHarness;
using Konveyor::Test::portalWarnings;

namespace
{

QString idOf(int number)
{
    return QStringLiteral("7656%1").arg(number, 3, 10, QLatin1Char('0'));
}

QString crowd(int count, int shift = 0)
{
    QJsonArray friends;
    for (int number = 0; number < count; ++number) {
        const int kind = (number + shift) % 4;
        const bool playing = kind == 0;
        friends.append(
            QJsonObject {{QStringLiteral("steamid"), idOf(number)}, {QStringLiteral("name"), QStringLiteral("Friend %1").arg(number)},
                {QStringLiteral("state"), kind == 3 ? 0 : 1}, {QStringLiteral("ingame"), playing},
                {QStringLiteral("appid"), playing ? QStringLiteral("%1").arg(500 + number % 3) : QString()},
                {QStringLiteral("game"), playing ? QStringLiteral("Game %1").arg(number % 3) : QString()},
                {QStringLiteral("chat"), QStringLiteral("steam://friends/message/") + idOf(number)}});
    }
    return QString::fromUtf8(QJsonDocument(QJsonObject {{QStringLiteral("ok"), true}, {QStringLiteral("friends"), friends}}).toJson());
}

}

class TestPortalFriendsRowsQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_harness = std::make_unique<PortalHarness>();
        QVERIFY(m_harness->load(QStringLiteral("org.devl0rd.portal.friends"), PortalHarness::Planar));
        m_harness->call("processSnapshot", crowd(60));
        m_window = std::make_unique<QQuickWindow>();
        m_window->resize(600, 800);
        auto *component = root()->property("fullRepresentation").value<QQmlComponent *>();
        m_view.reset(qobject_cast<QQuickItem *>(component->create(qmlContext(root()))));
        QVERIFY2(m_view, qPrintable(component->errorString()));
        m_view->setParentItem(m_window->contentItem());
        m_view->setSize(m_window->size());
        m_window->show();
        QTRY_VERIFY(list() && !shownRows().isEmpty());
        QVERIFY2(settled(), qPrintable(mismatch()));
    }

    void cleanup()
    {
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
        m_view.reset();
        m_window.reset();
        m_harness.reset();
    }

    void rowsShowTheirOwnFriendThroughEveryChange()
    {
        for (const char *tab : {"ingame", "online", "all"}) {
            root()->setProperty("tabKey", QLatin1String(tab));
            QVERIFY2(settled(), qPrintable(mismatch()));
        }
        m_harness->config()->insert(QStringLiteral("sortMode"), QStringLiteral("name_desc"));
        QVERIFY2(settled(), qPrintable(mismatch()));
        m_harness->config()->insert(QStringLiteral("hideOffline"), true);
        QTRY_COMPARE(rowCount(), 45);
        QVERIFY2(settled(), qPrintable(mismatch()));
        m_harness->config()->insert(QStringLiteral("hideOffline"), false);
        QTRY_COMPARE(rowCount(), 60);
        root()->setProperty("searchText", QStringLiteral("Friend 1"));
        QTRY_COMPARE(rowCount(), 11);
        QVERIFY2(settled(), qPrintable(mismatch()));
        root()->setProperty("searchText", QString());
        QTRY_COMPARE(rowCount(), 60);
        QVERIFY2(settled(), qPrintable(mismatch()));
        QVERIFY(QMetaObject::invokeMethod(list(), "positionViewAtEnd"));
        QVERIFY2(settled(), qPrintable(mismatch()));
        m_harness->call("toggleFavorite", idOf(59));
        QVERIFY2(settled(), qPrintable(mismatch()));
        for (int shift = 1; shift < 4; ++shift) {
            m_harness->call("processSnapshot", crowd(60 - shift * 7, shift));
            QTRY_COMPARE(rowCount(), 60 - shift * 7);
            QVERIFY2(settled(), qPrintable(mismatch()));
        }
    }

    void hoverAndActionsBelongToTheRowUnderThePointer()
    {
        const QString first = idAt(3);
        QQuickItem *row = rowFor(first);
        QVERIFY(row);
        const QPoint pointer = row->mapToScene(QPointF(row->width() / 3, row->height() / 2)).toPoint();
        QTest::mouseMove(m_window.get(), pointer);
        QTRY_COMPARE(hotIds(), QStringList {first});
        for (int shift = 1; shift < 5; ++shift) {
            m_harness->call("processSnapshot", crowd(60, shift));
            QVERIFY2(settled(), qPrintable(mismatch()));
            if (shift % 2) {
                QMetaObject::invokeMethod(list(), "positionViewAtIndex", Q_ARG(int, shift * 10), Q_ARG(int, 0));
                QVERIFY2(settled(), qPrintable(mismatch()));
            }
            QQuickItem *under = rowAt(pointer);
            QVERIFY(under);
            QTRY_COMPARE(hotIds(), QStringList {under->property("steamid").toString()});
        }
        QQuickItem *under = rowAt(pointer);
        const QString id = under->property("steamid").toString();
        const bool favourite = under->property("fav").toBool();
        QObject *star = withText(
            under->findChildren<QObject *>(), favourite ? QStringLiteral("Remove from Favourites") : QStringLiteral("Add to Favourites"));
        QVERIFY(star);
        QMetaObject::invokeMethod(star, "clicked");
        QCOMPARE(m_harness->call("isFavorite", id).toBool(), !favourite);
    }

    void menuActsOnTheFriendItWasOpenedFor()
    {
        const QString target = idAt(4);
        QPointer<QQuickItem> clicked = rowFor(target);
        QVERIFY(clicked);
        clickAt(clicked, Qt::RightButton);
        QObject *chat = nullptr;
        QTRY_VERIFY(chat = withText(m_view->findChildren<QObject *>(), QStringLiteral("Open Chat")));
        for (int shift = 1; shift < 4; ++shift) {
            m_harness->call("processSnapshot", crowd(60 - shift * 5, shift));
            QVERIFY2(settled(), qPrintable(mismatch()));
        }
        QVERIFY(QMetaObject::invokeMethod(list(), "positionViewAtEnd"));
        QVERIFY2(settled(), qPrintable(mismatch()));
        QMetaObject::invokeMethod(chat, "triggered");
        QVERIFY2(m_harness->requested().contains(QStringLiteral("steam 'steam://friends/message/%1'").arg(target)),
            qPrintable(m_harness->requested().join(QLatin1Char('\n'))));
    }

    void playingNowTilesFollowTheirGame()
    {
        const auto tiles = [&] {
            const QList<QQuickItem *> views = m_view->findChildren<QQuickItem *>();
            const auto strip = std::find_if(views.cbegin(), views.cend(), [](QQuickItem *view) {
                return view->inherits("QQuickListView") && view->property("orientation").toInt() == Qt::Horizontal && view->isVisible();
            });
            return strip == views.cend() ? QList<QQuickItem *>() : shownDelegates(*strip);
        };
        const auto tileMismatch = [&] {
            const QVariantList groups = root()->property("playingNow").toList();
            const QList<QQuickItem *> shown = tiles();
            for (QQuickItem *tile : shown) {
                const QVariantMap group = tile->property("modelData").toMap();
                const QStringList texts = plainTexts(tile);
                if (!texts.contains(group.value(QStringLiteral("game")).toString())
                    || !texts.contains(QString::number(group.value(QStringLiteral("friends")).toList().size()))) {
                    return texts.join(QLatin1Char('|'));
                }
            }
            return shown.size() == groups.size() ? QString() : QStringLiteral("%1 tiles for %2 games").arg(shown.size()).arg(groups.size());
        };
        QTRY_COMPARE(tileMismatch(), QString());
        for (int shift = 1; shift < 4; ++shift) {
            m_harness->call("processSnapshot", crowd(40 + shift * 5, shift));
            QTRY_COMPARE(tileMismatch(), QString());
        }
        QQuickItem *tile = tiles().value(1);
        const QString game = tile->property("modelData").toMap().value(QStringLiteral("game")).toString();
        clickAt(tile, Qt::LeftButton, QPointF(tile->width() / 2, 10));
        QTRY_COMPARE(root()->property("searchText").toString(), game);
        findByType(m_view.get(), "PopupShell").value(0)->setProperty("searchText", QString());
        QTRY_VERIFY(!tiles().isEmpty());
        tile = tiles().value(0);
        const QVariantMap player = tile->property("modelData").toMap().value(QStringLiteral("friends")).toList().value(1).toMap();
        const QList<QQuickItem *> faces = visibleItems(tile, "QQuickItem");
        const auto face = std::find_if(faces.cbegin(), faces.cend(), [&](QQuickItem *item) {
            return item->property("modelData").toMap().value(QStringLiteral("steamid")) == player.value(QStringLiteral("steamid"));
        });
        QVERIFY(face != faces.cend());
        clickAt(*face, Qt::LeftButton);
        QObject *chat = nullptr;
        QTRY_VERIFY(chat = withText(m_view->findChildren<QObject *>(), QStringLiteral("Open Chat")));
        m_harness->call("processSnapshot", crowd(60, 2));
        QMetaObject::invokeMethod(chat, "triggered");
        QVERIFY(m_harness->requested().contains(
            QStringLiteral("steam 'steam://friends/message/%1'").arg(player.value(QStringLiteral("steamid")).toString())));
    }

private:
    QObject *root() const { return m_harness->root(); }

    QString idAt(int index) const
    {
        const auto *rows = qobject_cast<QAbstractItemModel *>(root()->property("rows").value<QObject *>());
        return rows->data(rows->index(index, 0), rows->roleNames().key("steamid")).toString();
    }

    int rowCount() const { return root()->property("rows").value<QObject *>()->property("count").toInt(); }

    QQuickItem *list() const
    {
        const QList<QQuickItem *> views = m_view->findChildren<QQuickItem *>();
        for (QQuickItem *candidate : views) {
            if (candidate->inherits("QQuickListView")
                && candidate->property("model").value<QObject *>() == root()->property("rows").value<QObject *>()) {
                return candidate;
            }
        }
        return nullptr;
    }

    QList<QQuickItem *> shownRows() const
    {
        QQuickItem *view = list();
        return view ? shownDelegates(view) : QList<QQuickItem *>();
    }

    QString mismatch() const
    {
        const QVariantMap byId = root()->property("friendsById").toMap();
        for (QQuickItem *row : shownRows()) {
            const int index = delegateIndex(list(), row);
            const QString id = idAt(index);
            const QString name = byId.value(row->property("steamid").toString()).toMap().value(QStringLiteral("name")).toString();
            if (row->property("steamid").toString() != id || !plainTexts(row).contains(name)) {
                return QStringLiteral("row %1 should show %2 but shows %3").arg(index).arg(id, plainTexts(row).join(QLatin1Char('|')));
            }
        }
        return shownRows().isEmpty() ? QStringLiteral("no rows shown") : QString();
    }

    bool settled() const
    {
        return QQuickTest::qWaitForPolish(m_window.get()) && QTest::qWaitFor([&] { return mismatch().isEmpty(); });
    }

    QQuickItem *rowFor(const QString &id) const
    {
        const QList<QQuickItem *> rows = shownRows();
        const auto found
            = std::find_if(rows.cbegin(), rows.cend(), [&](QQuickItem *row) { return row->property("steamid").toString() == id; });
        return found == rows.cend() ? nullptr : *found;
    }

    QQuickItem *rowAt(const QPoint &point) const
    {
        const QList<QQuickItem *> rows = shownRows();
        const auto found
            = std::find_if(rows.cbegin(), rows.cend(), [&](QQuickItem *row) { return row->contains(row->mapFromScene(point)); });
        return found == rows.cend() ? nullptr : *found;
    }

    QStringList hotIds() const
    {
        QStringList ids;
        for (QQuickItem *row : shownRows()) {
            if (row->property("hot").toBool()) {
                ids.append(row->property("steamid").toString());
            }
        }
        return ids;
    }

    std::unique_ptr<PortalHarness> m_harness;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<QQuickItem> m_view;
};

QTEST_MAIN(TestPortalFriendsRowsQml)
#include "test_portal_friends_rows_qml.moc"
