#pragma once

#include <QByteArray>

namespace Konveyor::BorderShaders
{

QByteArray vertex();
QByteArray bandFragment();
QByteArray reactiveFragment();
QByteArray particleFragment();

}
