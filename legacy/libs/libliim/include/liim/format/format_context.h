#pragma once

#include <liim/string.h>
#include <liim/string_view.h>

namespace LIIM::Format {
class FormatContext {
public:
    using WriteFunction = void (*)(StringView, void*);

    constexpr explicit FormatContext(WriteFunction do_write, void* closure) : m_do_write(do_write), m_closure(closure) {}

    constexpr void put(StringView view) { m_do_write(view, m_closure); }

private:
    WriteFunction m_do_write { nullptr };
    void* m_closure { nullptr };
};
}
