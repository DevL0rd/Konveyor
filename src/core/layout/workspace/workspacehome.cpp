#include "layout/workspace/workspace.h"

namespace Konveyor::Layout
{

namespace
{

bool sameName(const QString &reference, const QString &name)
{
    return !reference.isEmpty() && reference.compare(name, Qt::CaseInsensitive) == 0;
}

}

bool outputMatches(const OutputArea &area, const QString &reference)
{
    return sameName(reference, area.outputName) || sameName(reference, area.outputId);
}

int Workspace::homeAffinity(const OutputArea &area) const
{
    if (!outputMatches(area, m_homeOutput)) {
        return 0;
    }
    return m_homeConnector.isEmpty() || sameName(m_homeConnector, area.outputName) ? 2 : 1;
}

void Workspace::makeHome(const OutputArea &area)
{
    m_homeOutput = area.outputId;
    m_homeConnector = area.outputName;
    m_homeUnplugged = false;
}

void Workspace::setConfiguredHome(const QString &reference)
{
    m_homeOutput = reference;
    m_homeConnector.clear();
    m_homeUnplugged = false;
    resolveHomeByConnector();
}

void Workspace::resolveHomeByConnector()
{
    if (sameName(m_homeOutput, m_area.outputName)) {
        makeHome(m_area);
    }
}

}
