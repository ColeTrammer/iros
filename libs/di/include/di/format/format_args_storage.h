#pragma once

#include <di/format/format_args.h>
#include <di/vocab/array/prelude.h>

namespace di::format {
template<size_t count, concepts::FormatArg Arg>
class FormatArgsStorage : public FormatArgs<Arg> {
public:
    using FormatArgs<Arg>::operator[];

    template<concepts::SameAs<Arg>... Args>
    requires(sizeof...(Args) == count)
    constexpr FormatArgsStorage(Args&&... args) : FormatArgs<Arg>({}), m_storage { util::forward<Args>(args)... } {
        this->set_args(m_storage.span());
    }

private:
    Array<Arg, count> m_storage;
};
}
