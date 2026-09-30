#pragma once

#include "layout/engine/engine.h"

#include <QList>
#include <QString>

namespace LayoutTest
{

namespace Layout = Konveyor::Layout;

constexpr int ActiveFullscreenStackingIndex = 1000000;

inline bool isVisibleTile(const Layout::WindowState &state)
{
    return state.visible && !state.isFloating && state.stackingIndex >= 0 && state.stackingIndex <= ActiveFullscreenStackingIndex
        && state.renderFrame.width() > 0.0 && state.renderFrame.height() > 0.0;
}

inline QRectF settledRect(const Layout::WindowState &state)
{
    const QSizeF size = state.renderFrame.size().boundedTo(state.targetFrame.size());
    return QRectF(state.renderFrame.topLeft(), size).adjusted(1, 1, -1, -1);
}

inline QString overlappingTiles(const QList<Layout::WindowState> &states)
{
    for (qsizetype i = 0; i < states.size(); ++i) {
        const Layout::WindowState &a = states.at(i);
        if (!isVisibleTile(a)) {
            continue;
        }
        for (qsizetype j = i + 1; j < states.size(); ++j) {
            const Layout::WindowState &b = states.at(j);
            if (!isVisibleTile(b) || a.workspace != b.workspace) {
                continue;
            }
            const QRectF first = settledRect(a);
            const QRectF second = settledRect(b);
            if (first.isValid() && second.isValid() && first.intersects(second)) {
                return QStringLiteral("tiles %1 and %2 overlap on workspace %3").arg(a.id).arg(b.id).arg(a.workspace);
            }
        }
    }
    return {};
}

inline QString tileProblems(const Layout::Engine &engine)
{
    if (engine.isAnimating()) {
        return {};
    }
    return overlappingTiles(engine.windowStates());
}

}
