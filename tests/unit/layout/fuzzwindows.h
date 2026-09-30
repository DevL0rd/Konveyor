#pragma once

#include "fuzzevents.h"

namespace LayoutTest
{

inline Layout::WindowProperties fuzzWindowProperties(const Event &event, std::optional<Layout::WindowId> parent)
{
    static const QStringList apps {QStringLiteral("app"), QStringLiteral("term"), QStringLiteral("game"), QStringLiteral("dialog"),
        QStringLiteral("browser"), QStringLiteral("app")};
    const QString app = apps.at(static_cast<qsizetype>(event.a % apps.size()));
    Layout::WindowProperties properties = makeWindow(app, app, QSizeF(60 + event.b % 1400, 40 + event.c % 1000));
    switch ((event.a >> 4) % 6) {
    case 0:
        properties.minSize = QSizeF(400, 300);
        break;
    case 1:
        properties.maxSize = QSizeF(600, 500);
        break;
    case 2:
        properties.isResizable = false;
        break;
    case 3:
        properties.frameSize = QSizeF(10, 10);
        properties.minSize = QSizeF(5, 5);
        break;
    default:
        break;
    }
    properties.parent = parent;
    properties.isDialog = (event.a >> 16) % 10 == 0;
    properties.wantsFullscreen = (event.a >> 20) % 12 == 0;
    properties.wantsMaximized = (event.a >> 24) % 12 == 0;
    properties.isUrgent = (event.b >> 20) % 15 == 0;
    return properties;
}

inline Client fuzzClient(const Layout::WindowProperties &properties, quint32 kind)
{
    Client client;
    switch (kind % 5) {
    case 1:
        client = Client::honoring(properties);
        break;
    case 2:
        client.commitsLate = true;
        break;
    case 3:
        client.maxSize = QSizeF(300 + (kind >> 3) % 700, 0);
        break;
    case 4:
        client = Client::honoring(properties);
        client.commitsLate = true;
        break;
    default:
        break;
    }
    return client;
}

inline Layout::WindowProperties changedProperties(Layout::WindowProperties properties, quint32 variant)
{
    properties.title += QStringLiteral("*");
    switch (variant % 4) {
    case 0:
        properties.minSize = QSizeF(800, 600);
        break;
    case 1:
        properties.isResizable = !properties.isResizable;
        break;
    case 2:
        properties.wantsFullscreen = !properties.wantsFullscreen;
        break;
    default:
        properties.frameSize = QSizeF(200 + variant % 900, 150 + variant % 700);
        break;
    }
    return properties;
}

}
