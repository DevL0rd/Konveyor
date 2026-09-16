#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>

namespace Konveyor
{

class PlasmaShellSync : public QObject
{
    Q_OBJECT

public:
    explicit PlasmaShellSync(QObject *parent = nullptr);

    void start();
    void update(const QList<Layout::WindowState> &states, const QStringList &outputOrder);
    void setHideDesktopWidgets(bool enabled);
    void setFillPanels(bool enabled);

private:
    struct ScreenState
    {
        bool occupied = false;
        bool expanded = false;
        bool operator==(const ScreenState &) const = default;
    };

    void scheduleApply();
    void apply();

    QTimer m_debounce;
    QHash<QString, ScreenState> m_screens;
    QStringList m_outputOrder;
    bool m_hideDesktopWidgets = false;
    bool m_fillPanels = false;
    bool m_started = false;
    bool m_seenLayout = false;
};

}
