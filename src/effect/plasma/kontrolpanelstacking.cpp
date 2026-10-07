#include "plasma/kontrolpanelstacking.h"

#include "config/log.h"

#include <KConfig>
#include <KConfigGroup>

#include <window.h>
#include <workspace.h>

#include <QScopedValueRollback>

namespace Konveyor
{

namespace
{

bool isKontrolPanel(KWin::Window *window, KWin::Layer layer)
{
    return window->resourceClass() == QLatin1String("konveyor-kontrol-panel") && window->layer() == layer;
}

bool isKontrolPanelBackdrop(KWin::Window *window)
{
    return isKontrolPanel(window, KWin::AboveLayer);
}

bool isKontrolPanelCard(KWin::Window *window)
{
    return isKontrolPanel(window, KWin::OverlayLayer);
}

bool taskbarUsableWhileOpen()
{
    const KConfig config(QStringLiteral("konveyor/kontrolpanelrc"), KConfig::SimpleConfig);
    return config.group(QStringLiteral("General")).readEntry("useTaskbarWhileOpen", true);
}

}

KontrolPanelStacking::KontrolPanelStacking(QObject *parent)
    : QObject(parent)
{ }

void KontrolPanelStacking::start()
{
    connect(KWin::workspace(), &KWin::Workspace::windowAdded, this, &KontrolPanelStacking::onWindowAdded);
    connect(KWin::workspace(), &KWin::Workspace::stackingOrderChanged, this, &KontrolPanelStacking::keepInputMethodsAbovePanel);
}

void KontrolPanelStacking::onWindowAdded(KWin::Window *window)
{
    if (isKontrolPanelBackdrop(window) && taskbarUsableWhileOpen()) {
        qCInfo(lcKonveyor) << "konveyor: lowering the Kontrol Panel backdrop below panels";
        KWin::workspace()->lowerWindow(window);
    }
}

void KontrolPanelStacking::keepInputMethodsAbovePanel()
{
    if (m_restacking) {
        return;
    }
    const QList<KWin::Window *> &order = KWin::workspace()->stackingOrder();
    qsizetype panelTop = -1;
    for (qsizetype i = 0; i < order.size(); ++i) {
        if (isKontrolPanelCard(order[i])) {
            panelTop = i;
        }
    }
    if (panelTop < 0) {
        return;
    }
    QList<KWin::Window *> buried;
    for (qsizetype i = 0; i < panelTop; ++i) {
        if (order[i]->isInputMethod()) {
            buried.append(order[i]);
        }
    }
    const QScopedValueRollback guard(m_restacking, true);
    for (KWin::Window *window : std::as_const(buried)) {
        qCInfo(lcKonveyor) << "konveyor: raising input method" << window->resourceClass() << "above the Kontrol Panel";
        KWin::workspace()->raiseWindow(window);
    }
}

}
