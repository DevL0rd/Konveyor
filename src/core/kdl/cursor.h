#pragma once

#include "kdl/kdl.h"

#include <string>
#include <string_view>

namespace Konveyor::Kdl
{

class Cursor
{
public:
    struct Mark
    {
        qsizetype position = 0;
        int line = 1;
        int column = 1;
    };

    Cursor(const QString &text, const QString &fileName);

    bool atEnd() const;
    char32_t peek(qsizetype offset = 0) const;
    bool lookingAt(std::u32string_view sequence) const;
    qsizetype position() const;
    std::u32string_view slice(qsizetype from) const;
    Location location() const;
    Mark mark() const;
    void reset(const Mark &mark);
    void advance(qsizetype count = 1);

private:
    void step();

    std::u32string m_text;
    QString m_fileName;
    qsizetype m_position = 0;
    int m_line = 1;
    int m_column = 1;
};

}
