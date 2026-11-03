#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QSet>
#include <QTimer>

#include <memory>

namespace Konveyor::Settings
{

class BorderStyleItem;

class BorderStyleClock : public QObject
{
    Q_OBJECT

public:
    BorderStyleClock();

    static std::shared_ptr<BorderStyleClock> shared();

    void setAnimating(BorderStyleItem *item, bool animating);
    double seconds() const;

private:
    void tick();

    QElapsedTimer m_elapsed;
    QTimer m_timer;
    QSet<BorderStyleItem *> m_items;
};

}
