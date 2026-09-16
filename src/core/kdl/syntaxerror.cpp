#include "kdl/syntaxerror.h"

#include <utility>

namespace Konveyor::Kdl
{

SyntaxError::SyntaxError(ParseError error)
    : m_error(std::move(error))
{ }

const ParseError &SyntaxError::error() const
{
    return m_error;
}

const char *SyntaxError::what() const noexcept
{
    return "KDL syntax error";
}

void raiseSyntaxError(const QString &message, const Location &location)
{
    throw SyntaxError(ParseError {message, location});
}

}
