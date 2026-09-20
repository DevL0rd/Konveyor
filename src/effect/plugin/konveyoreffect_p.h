#pragma once

#include "plugin/konveyoreffect.h"

#include "dbus/dbusservice.h"
#include "decorations/accentcolor.h"
#include "decorations/decorationlayer.h"
#include "decorations/fullscreenshade.h"
#include "input/gesturefilter.h"
#include "input/inputfilter.h"
#include "input/shortcutmanager.h"
#include "input/spillinputfilter.h"
#include "input/touchpadcontactreader.h"
#include "kwin/desktopsync.h"
#include "kwin/fullscreenguard.h"
#include "kwin/minimizerule.h"
#include "kwin/outputregistry.h"
#include "kwin/windowapplier.h"
#include "kwin/windowregistry.h"
#include "plasma/plasmashellsync.h"
#include "plugin/configmanager.h"
#include "plugin/windowmemorystore.h"

#include "anim/clock.h"
#include "config/loader.h"
#include "ipc/model.h"
#include "layout/gestures/gesturerouter.h"

#include <core/output.h>
#include <effect/effecthandler.h>
#include <input.h>
#include <wayland/seat.h>
#include <wayland_server.h>
#include <window.h>
#include <workspace.h>

#include <algorithm>
#include <chrono>
#include <utility>

#include <QJsonArray>
#include <QDir>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

namespace Konveyor
{

inline constexpr int animationIntervalMs = 8;
inline constexpr int memorySaveDelayMs = 1000;
inline constexpr int interactivePhaseStart = 0;
inline constexpr int interactivePhaseStep = 1;
inline constexpr int interactivePhaseEnd = 2;

inline QString outputNameOf(KWin::Window *window)
{
    return window->output() ? window->output()->name() : QString();
}

inline QString outputNameAt(const QPointF &position)
{
    const KWin::LogicalOutput *output = KWin::workspace()->outputAt(position);
    return output ? output->name() : QString();
}

inline QList<std::pair<KWin::ElectricBorder, bool Config::HotCorners::*>> hotCornerFlags()
{
    return {
        {KWin::ElectricTopLeft, &Config::HotCorners::topLeft},
        {KWin::ElectricTopRight, &Config::HotCorners::topRight},
        {KWin::ElectricBottomLeft, &Config::HotCorners::bottomLeft},
        {KWin::ElectricBottomRight, &Config::HotCorners::bottomRight},
    };
}

struct KonveyorEffect::Private
{
    Anim::Clock clock;
    ConfigManager config;
    AccentColor accent;
    WindowRegistry windows;
    FullscreenGuard fullscreenGuard;
    OutputRegistry outputs;
    Layout::Engine engine;
    Layout::GestureRouter gestures;
    WindowApplier applier;
    DecorationLayer decorations;
    FullscreenShade fullscreenShade;
    DesktopSync desktops;
    ShortcutManager shortcuts;
    PlasmaShellSync plasmaShell;
    std::unique_ptr<SpillInputFilter> spillInput;
    std::unique_ptr<InputFilter> input;
    std::unique_ptr<GestureFilter> gestureInput;
    std::unique_ptr<TouchpadContactReader> touchpadContacts;
    std::unique_ptr<DBusService> dbus;
    QTimer flushTimer;
    QTimer animationTimer;
    QTimer memorySaveTimer;
    WindowMemoryStore memoryStore;
    bool animating = false;
    QSet<KWin::ElectricBorder> reservedCorners;
    double dragOrigin = 0;
    QSizeF resizeOrigin;
    quint64 bindCount = 0;
    QString lastBindKey;
    QString lastBindAction;
    std::optional<Layout::WindowId> focusRequest;
    std::optional<Layout::WindowId> titlebarDrag;
    std::optional<Layout::WindowId> touchLift;
    std::optional<Layout::WindowId> touchMovePending;
    QHash<Layout::WindowId, QString> homeOutputs;
    QHash<KWin::Window *, Layout::RestorePlacement> minimizedPlacements;
    QHash<Layout::WindowId, QHash<int, QPointer<KWin::Window>>> monitorOverlays;
    QHash<Layout::WindowId, QHash<int, QPointer<KWin::Window>>> monitorPanels;
    QSet<Layout::WindowId> placingMonitorOverlays;
    QSet<Layout::WindowId> placingMonitorPanels;

    Private(Layout::Hooks hooks, ShortcutManager::Handler shortcutHandler)
        : fullscreenGuard(windows)
        , engine(clock, std::move(hooks))
        , gestures(engine)
        , applier(windows)
        , decorations(accent)
        , fullscreenShade(windows)
        , desktops(windows, outputs)
        , shortcuts(std::move(shortcutHandler))
    { }
};
}
