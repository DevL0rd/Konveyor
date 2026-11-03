#pragma once

#include "settingsqmlharness.h"

#include <QQuickItem>
#include <QQuickWindow>

namespace Konveyor::Settings::Testing
{

struct StyleScene
{
    std::unique_ptr<QQuickWindow> window;
    QQuickItem *item = nullptr;
};

inline StyleScene openStyleScene(SettingsHome &home, QQmlEngine &engine, const QByteArray &window)
{
    StyleScene scene;
    QString error;
    const QByteArray body = QByteArray("import QtQuick.Window\n") + window;
    std::unique_ptr<QObject> created = home.create(engine, body, &error);
    if (!created) {
        QTest::qFail(qPrintable(error), __FILE__, __LINE__);
        return scene;
    }
    scene.window.reset(qobject_cast<QQuickWindow *>(created.release()));
    scene.item = scene.window->findChild<QQuickItem *>("style");
    if (!QTest::qWaitForWindowExposed(scene.window.get())) {
        QTest::qFail("the window was not exposed", __FILE__, __LINE__);
    }
    return scene;
}

}
