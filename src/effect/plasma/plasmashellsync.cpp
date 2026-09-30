#include "plasma/plasmashellsync.h"

#include <effect/effecthandler.h>
#include <window.h>
#include <workspace.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

namespace Konveyor
{

namespace
{

constexpr int applyDebounceMs = 80;

const QString &syncScript()
{
    static const QString script = QStringLiteral(R"(
var hideWidgets = %1;
var fillPanels = %2;
for (const desktop of desktops()) {
    if (!(desktop.screen in hideWidgets)) { continue; }
    desktop.currentConfigGroup = ["General"];
    desktop.writeConfig("hideDesktopWidgets", hideWidgets[desktop.screen]);
}
for (const panel of panels()) {
    const fill = fillPanels[panel.screen] === true;
    panel.currentConfigGroup = ["Konveyor"];
    const saved = panel.readConfig("savedLengthMode", "");
    if (fill && saved === "" && panel.lengthMode !== "fill") {
        panel.writeConfig("savedLengthMode", panel.lengthMode);
        panel.lengthMode = "fill";
    } else if (!fill && saved !== "") {
        if (panel.lengthMode === "fill") {
            panel.lengthMode = saved;
        }
        panel.writeConfig("savedLengthMode", "");
    }
}
)");
    return script;
}

QString screenMap(const QList<bool> &values)
{
    QStringList entries;
    for (qsizetype screen = 0; screen < values.size(); ++screen) {
        entries.append(QStringLiteral("%1:%2").arg(screen).arg(values.at(screen) ? QStringLiteral("true") : QStringLiteral("false")));
    }
    return QLatin1Char('{') + entries.join(QLatin1Char(',')) + QLatin1Char('}');
}

void callPlasma(const QString &script)
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"), QStringLiteral("/PlasmaShell"),
        QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("evaluateScript"));
    message.setArguments({script});
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message));
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [](QDBusPendingCallWatcher *call) {
        if (call->isError()) {
            qWarning() << "konveyor: plasmashell did not apply the panel and widget settings:" << call->error().message();
        }
        call->deleteLater();
    });
}

}

PlasmaShellSync::PlasmaShellSync(QObject *parent)
    : QObject(parent)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(applyDebounceMs);
    connect(&m_debounce, &QTimer::timeout, this, &PlasmaShellSync::apply);
}

PlasmaShellSync::~PlasmaShellSync()
{
    if (m_started && m_seenLayout) {
        send(false, false);
    }
}

void PlasmaShellSync::start()
{
    m_started = true;
    connect(KWin::effects, &KWin::EffectsHandler::showingDesktopChanged, this, &PlasmaShellSync::scheduleApply);
    m_shellWatcher.setConnection(QDBusConnection::sessionBus());
    m_shellWatcher.setWatchMode(QDBusServiceWatcher::WatchForRegistration);
    m_shellWatcher.addWatchedService(QStringLiteral("org.kde.plasmashell"));
    connect(&m_shellWatcher, &QDBusServiceWatcher::serviceRegistered, this, &PlasmaShellSync::scheduleApply);
    connect(KWin::workspace(), &KWin::Workspace::windowAdded, this, [this](KWin::Window *window) {
        if (window->isDock()) {
            connect(window, &KWin::Window::outputChanged, this, &PlasmaShellSync::scheduleApply);
            scheduleApply();
        }
    });
    scheduleApply();
}

void PlasmaShellSync::setHideDesktopWidgets(bool enabled)
{
    if (m_hideDesktopWidgets != enabled) {
        m_hideDesktopWidgets = enabled;
        scheduleApply();
    }
}

void PlasmaShellSync::setFillPanels(bool enabled)
{
    if (m_fillPanels != enabled) {
        m_fillPanels = enabled;
        scheduleApply();
    }
}

void PlasmaShellSync::update(const QList<Layout::WindowState> &states, const QStringList &outputOrder)
{
    QHash<QString, ScreenState> screens;
    for (const QString &output : outputOrder) {
        screens.insert(output, {});
    }
    for (const Layout::WindowState &state : states) {
        if (!state.visible || !state.onActiveWorkspace || state.output.isEmpty()) {
            continue;
        }
        ScreenState &screen = screens[state.output];
        screen.occupied = true;
        screen.expanded = screen.expanded || state.sizingMode == Layout::WindowMode::Maximized;
    }
    m_seenLayout = true;
    if (screens == m_screens && outputOrder == m_outputOrder) {
        return;
    }
    m_screens = screens;
    m_outputOrder = outputOrder;
    scheduleApply();
}

void PlasmaShellSync::scheduleApply()
{
    if (m_started && m_seenLayout) {
        m_debounce.start();
    }
}

void PlasmaShellSync::apply()
{
    send(m_hideDesktopWidgets && !KWin::workspace()->showingDesktop(), m_fillPanels);
}

void PlasmaShellSync::send(bool hideWidgets, bool fillPanels)
{
    QList<bool> hidden;
    QList<bool> filled;
    for (const QString &output : std::as_const(m_outputOrder)) {
        const ScreenState screen = m_screens.value(output);
        hidden.append(hideWidgets && screen.occupied);
        filled.append(fillPanels && screen.expanded);
    }
    callPlasma(syncScript().arg(screenMap(hidden), screenMap(filled)));
}

}
