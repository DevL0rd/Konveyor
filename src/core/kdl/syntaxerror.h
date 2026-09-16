#pragma once

#include "kdl/kdl.h"

#include <exception>

namespace Konveyor::Kdl
{

class SyntaxError : public std::exception
{
public:
    explicit SyntaxError(ParseError error);

    const ParseError &error() const;
    const char *what() const noexcept override;

private:
    ParseError m_error;
};

[[noreturn]] void raiseSyntaxError(const QString &message, const Location &location);

}
