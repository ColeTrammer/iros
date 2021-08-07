#pragma once

#include <liim/forward.h>

namespace LIIM::Format {
class FormatArg final {
public:
    FormatArg(const void* value, void (*do_format)(const void*, FormatContext&)) : m_value(value), m_do_format(do_format) {}

    void do_format(FormatContext& context) { return m_do_format(m_value, context); }

private:
    const void* m_value { nullptr };
    void (*m_do_format)(const void*, FormatContext&) { nullptr };
};
}
