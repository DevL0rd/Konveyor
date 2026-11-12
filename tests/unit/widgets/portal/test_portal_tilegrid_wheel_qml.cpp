#include <QGuiApplication>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QWheelEvent>

#include <memory>

namespace
{

const char *const sourceTemplate = R"(
import QtQuick
import "@LAUNCHER@"

Item {
    id: launcher
    width: 300
    height: 300
    property bool activationPending: false
    property alias outer: outer
    property alias grid: grid
    property alias scrollingGrid: scrollingGrid
    function ensureSelection() {}
    function activatePending() {}

    Flickable {
        id: outer
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.height
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: column
            width: 300

            TileGrid {
                id: grid
                width: 300
                height: 200
                cellWidth: 100
                cellHeight: 100
                scrolling: false
                model: 12
                delegate: Rectangle { width: 100; height: 100 }
            }

            TileGrid {
                id: scrollingGrid
                width: 300
                height: 100
                cellWidth: 100
                cellHeight: 100
                scrolling: true
                model: 12
                delegate: Rectangle { width: 100; height: 100 }
            }

            Item { width: 300; height: 400 }
        }
    }
}
)";

QByteArray qmlSource()
{
    return QByteArray(sourceTemplate).replace("@LAUNCHER@", "file://" KONVEYOR_SOURCE_DIR "/widgets/portals/shared/launcher");
}

void wheel(QQuickWindow *window, const QPointF &position)
{
    QWheelEvent event(position, window->mapToGlobal(position), QPoint(), QPoint(0, -240), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(window, &event);
}

}

class TestPortalTileGridWheelQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_engine = std::make_unique<QQmlEngine>();
        QQmlComponent component(m_engine.get());
        component.setData(qmlSource(), QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        m_root.reset(qobject_cast<QQuickItem *>(component.create()));
        QVERIFY(m_root);
        m_window = std::make_unique<QQuickWindow>();
        m_window->resize(300, 300);
        m_root->setParentItem(m_window->contentItem());
        m_window->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
    }

    void cleanup()
    {
        m_window.reset();
        m_root.reset();
        m_engine.reset();
    }

    void staticGridPassesTheWheelToTheOuterFlickable()
    {
        QObject *outer = m_root->property("outer").value<QObject *>();
        QObject *grid = m_root->property("grid").value<QObject *>();
        QCOMPARE(outer->property("contentY").toReal(), 0.0);
        wheel(m_window.get(), QPointF(150, 100));
        QTRY_VERIFY(outer->property("contentY").toReal() > 0.0);
        QCOMPARE(grid->property("contentY").toReal(), 0.0);
    }

    void scrollingGridScrollsItself()
    {
        QObject *outer = m_root->property("outer").value<QObject *>();
        QObject *scrolling = m_root->property("scrollingGrid").value<QObject *>();
        QCOMPARE(scrolling->property("contentY").toReal(), 0.0);
        outer->setProperty("contentY", 200.0);
        QCOMPARE(outer->property("contentY").toReal(), 200.0);
        wheel(m_window.get(), QPointF(150, 50));
        QTRY_VERIFY(scrolling->property("contentY").toReal() > 0.0);
        QCOMPARE(outer->property("contentY").toReal(), 200.0);
    }

private:
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QQuickItem> m_root;
    std::unique_ptr<QQuickWindow> m_window;
};

QTEST_MAIN(TestPortalTileGridWheelQml)
#include "test_portal_tilegrid_wheel_qml.moc"
