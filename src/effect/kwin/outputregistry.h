#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QObject>
#include <QStringList>

namespace KWin
{
class LogicalOutput;
}

namespace Konveyor
{

class OutputRegistry : public QObject
{
    Q_OBJECT

public:
    explicit OutputRegistry(QObject *parent = nullptr);

    void start();

    static Layout::OutputInfo infoOf(KWin::LogicalOutput *output);
    KWin::LogicalOutput *outputNamed(const QString &name) const;
    QList<Layout::OutputInfo> outputs() const;
    QStringList orderedNames() const;

Q_SIGNALS:
    void outputAdded(const Layout::OutputInfo &info);
    void outputChanged(const Layout::OutputInfo &info);
    void outputRemoved(const QString &name);
    void activeOutputChanged(const QString &name);

private:
    void scheduleRefresh();
    void refresh();

    QHash<QString, Layout::OutputInfo> m_known;
    bool m_refreshQueued = false;
};

}
