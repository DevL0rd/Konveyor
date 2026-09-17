#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QStringList>

namespace Konveyor::Settings
{

class EditHistory
{
public:
    void record(const QString &before);
    std::optional<QString> undo();
    void clear();
    bool canUndo() const;

private:
    QStringList m_snapshots;
    QElapsedTimer m_lastRecord;
};

}
