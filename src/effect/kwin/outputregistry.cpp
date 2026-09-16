#include "kwin/outputregistry.h"

#include <core/output.h>
#include <workspace.h>

#include <QTimer>

namespace Konveyor
{

OutputRegistry::OutputRegistry(QObject *parent)
    : QObject(parent)
{ }

void OutputRegistry::start()
{
    const auto queue = [this]() { scheduleRefresh(); };
    connect(KWin::workspace(), &KWin::Workspace::outputsChanged, this, queue);
    connect(KWin::workspace(), &KWin::Workspace::geometryChanged, this, queue);
    connect(KWin::workspace(), &KWin::Workspace::aboutToRearrange, this, queue);
    connect(KWin::workspace(), &KWin::Workspace::activeOutputChanged, this, [this](KWin::LogicalOutput *output) {
        if (output) {
            Q_EMIT activeOutputChanged(output->name());
        }
    });
    refresh();
}

Layout::OutputInfo OutputRegistry::infoOf(KWin::LogicalOutput *output)
{
    Layout::OutputInfo info;
    info.name = output->name();
    info.makeModelSerial = QStringList {output->manufacturer(), output->model(), output->serialNumber()}.join(QLatin1Char(' ')).trimmed();
    info.geometry = output->geometryF();
    info.workArea = KWin::workspace()->clientArea(KWin::MaximizeArea, output);
    info.scale = output->scale();
    return info;
}

KWin::LogicalOutput *OutputRegistry::outputNamed(const QString &name) const
{
    const QList<KWin::LogicalOutput *> outputs = KWin::workspace()->outputs();
    const auto it = std::ranges::find_if(outputs, [&name](KWin::LogicalOutput *output) { return output->name() == name; });
    return it == outputs.end() ? nullptr : *it;
}

QStringList OutputRegistry::orderedNames() const
{
    QStringList names;
    for (KWin::LogicalOutput *output : KWin::workspace()->outputs()) {
        names.append(output->name());
    }
    return names;
}

QList<Layout::OutputInfo> OutputRegistry::outputs() const
{
    return m_known.values();
}

void OutputRegistry::scheduleRefresh()
{
    if (m_refreshQueued) {
        return;
    }
    m_refreshQueued = true;
    QTimer::singleShot(0, this, [this]() {
        m_refreshQueued = false;
        refresh();
    });
}

void OutputRegistry::refresh()
{
    QHash<QString, Layout::OutputInfo> current;
    const QList<KWin::LogicalOutput *> outputs = KWin::workspace()->outputs();
    for (KWin::LogicalOutput *output : outputs) {
        current.insert(output->name(), infoOf(output));
    }
    for (auto it = m_known.cbegin(); it != m_known.cend(); ++it) {
        if (!current.contains(it.key())) {
            Q_EMIT outputRemoved(it.key());
        }
    }
    for (auto it = current.cbegin(); it != current.cend(); ++it) {
        const auto previous = m_known.constFind(it.key());
        if (previous == m_known.cend()) {
            Q_EMIT outputAdded(*it);
        } else if (*previous != *it) {
            Q_EMIT outputChanged(*it);
        }
    }
    m_known = current;
}

}
