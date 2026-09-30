#pragma once

#include "helpers.h"

#include <QMap>

namespace LayoutTest
{

using WindowIds = QList<Layout::WindowId>;
using Columns = QList<WindowIds>;

inline Layout::ActionResult act(
    Fixture &fixture, const QString &name, const QStringList &arguments = {}, const QList<std::pair<QString, QString>> &properties = {})
{
    const Layout::ActionResult result = fixture.perform(name, arguments, properties);
    fixture.advance(1);
    return result;
}

inline Columns columnsOn(Fixture &fixture, const QString &output, int workspaceIndex)
{
    QMap<int, QMap<int, Layout::WindowId>> grid;
    for (const Layout::WindowState &state : fixture.engine().windowStates()) {
        if (state.output == output && state.workspaceIndex == workspaceIndex && !state.isFloating) {
            grid[state.columnIndex].insert(state.tileIndex, state.id);
        }
    }
    Columns columns;
    for (const QMap<int, Layout::WindowId> &column : std::as_const(grid)) {
        columns.append(column.values());
    }
    return columns;
}

inline Columns columns(Fixture &fixture)
{
    return columnsOn(fixture, QStringLiteral("DP-1"), 1);
}

inline QList<Layout::WorkspaceState> workspacesOn(Fixture &fixture, const QString &output)
{
    QList<Layout::WorkspaceState> result;
    for (const Layout::WorkspaceState &state : fixture.engine().workspaceStates()) {
        if (state.output == output) {
            result.append(state);
        }
    }
    return result;
}

inline QStringList namesOn(Fixture &fixture, const QString &output)
{
    QStringList names;
    for (const Layout::WorkspaceState &state : workspacesOn(fixture, output)) {
        names.append(state.name);
    }
    return names;
}

inline int activeWorkspaceOn(Fixture &fixture, const QString &output)
{
    for (const Layout::WorkspaceState &state : workspacesOn(fixture, output)) {
        if (state.isActive) {
            return state.index;
        }
    }
    return 0;
}

inline std::pair<QString, int> placeOf(Fixture &fixture, Layout::WindowId id)
{
    const Layout::WindowState state = fixture.state(id);
    return {state.output, state.workspaceIndex};
}

inline Layout::WindowId addOn(Fixture &fixture, const QString &output, const QString &appId = {})
{
    fixture.engine().focusOutput(output);
    return fixture.add(appId);
}

inline void addOutputAt(Fixture &fixture, const QString &name, QRectF geometry)
{
    fixture.engine().addOutput(makeOutput(name, geometry));
    fixture.settle();
}

inline QString focusedOutput(Fixture &fixture)
{
    return fixture.engine().focusedOutput().value_or(QString());
}

inline WindowIds stackOf(Fixture &fixture, int count)
{
    WindowIds ids;
    for (int idx = 0; idx < count; ++idx) {
        ids.append(fixture.add(QStringLiteral("stack")));
        if (idx > 0) {
            fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        }
    }
    return ids;
}

inline WindowIds columnsOf(Fixture &fixture, int count)
{
    WindowIds ids;
    for (int idx = 0; idx < count; ++idx) {
        ids.append(fixture.add(QStringLiteral("column")));
    }
    return ids;
}

inline Layout::WindowId addFloating(Fixture &fixture)
{
    const Layout::WindowId id = fixture.add(QStringLiteral("floating"));
    fixture.perform(QStringLiteral("move-window-to-floating"));
    return id;
}

inline bool isInside(const QRectF &frame, const QRectF &area)
{
    return area.contains(frame);
}

}

#define COMPARE_FOCUS(fixture, expected) QCOMPARE((fixture).focused(), std::optional<Layout::WindowId>(expected))
