#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QStringList>

#include <chrono>
#include <optional>

namespace Konveyor::Settings
{

class EditHistory
{
public:
    explicit EditHistory(std::chrono::milliseconds burst = std::chrono::milliseconds(700));

    void record(const QString &before);
    std::optional<QString> undo();
    void clear();
    bool canUndo() const;

private:
    std::chrono::milliseconds m_burst;
    QStringList m_snapshots;
    QElapsedTimer m_lastRecord;
};

}
