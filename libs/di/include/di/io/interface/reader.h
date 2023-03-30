#pragma once

#include <di/any/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::io {
namespace detail {
    struct ReadSomeMember {
        template<typename T>
        constexpr Result<usize> operator()(T& reader, Span<Byte> data) const
        requires(requires {
            { reader.read_some(data) } -> concepts::ImplicitlyConvertibleTo<Result<usize>>;
        })
        {
            return reader.read_some(data);
        }
    };
}

struct ReadSome : Dispatcher<ReadSome, Result<usize>(This&, Span<Byte>), detail::ReadSomeMember> {};

constexpr inline auto read_some = ReadSome {};

using Reader = meta::List<ReadSome>;
}
