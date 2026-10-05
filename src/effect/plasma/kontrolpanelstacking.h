#pragma once

#include <QObject>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class KontrolPanelStacking : public QObject
{
    Q_OBJECT

public:
    explicit KontrolPanelStacking(QObject *parent = nullptr);

    void start();

private:
    void onWindowAdded(KWin::Window *window);
    void keepInputMethodsAbovePanel();

    bool m_restacking = false;
};

}
