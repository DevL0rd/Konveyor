#pragma once

#include <KConfigWatcher>

#include "config/types.h"

#include <QColor>
#include <QHash>
#include <QObject>

namespace Konveyor
{

class AccentColor : public QObject
{
    Q_OBJECT

public:
    explicit AccentColor(QObject *parent = nullptr);

    QColor colorFor(Config::ColorSource source) const;

Q_SIGNALS:
    void changed();

private:
    void reload();

    KConfigWatcher::Ptr m_watcher;
    QColor m_color;
    QHash<int, QColor> m_colors;
};

}
