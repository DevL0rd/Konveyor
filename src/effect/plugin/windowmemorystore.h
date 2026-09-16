#pragma once

#include "layout/engine/windowmemory.h"

#include <QString>

namespace Konveyor
{

class WindowMemoryStore
{
public:
    WindowMemoryStore();

    Layout::WindowMemory load() const;
    void save(const Layout::WindowMemory &memory) const;

private:
    QString m_path;
};

}
