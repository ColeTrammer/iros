#pragma once

#include <liim/string.h>
#include <liim/string_view.h>

namespace LIIM::Format {
class FormatContext {
public:
    void put(StringView view) { m_accumulator += String(view); }

    String take_accumulator() { return move(m_accumulator); }

private:
    String m_accumulator;
};
}
