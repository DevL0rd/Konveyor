#include "decorations/bordershaders.h"

#include "bordershadersources.h"

namespace Konveyor::BorderShaders
{

QByteArray vertex()
{
    return kVertexSource;
}

QByteArray bandFragment()
{
    return kBandFragmentSource;
}

QByteArray reactiveFragment()
{
    return kReactiveFragmentSource;
}

QByteArray particleFragment()
{
    return kParticleFragmentSource;
}

}
