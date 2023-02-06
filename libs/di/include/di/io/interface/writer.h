#pragma once

#include <di/any/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::io {
namespace detail {
    struct WriteSomeMember {
        template<typename T>
        constexpr Result<usize> operator()(T& writer, Span<Byte const> data) const
        requires(requires {
                     { writer.write_some(data) } -> concepts::ImplicitlyConvertibleTo<Result<usize>>;
                 })
        {
            return writer.write_some(data);
        }
    };

    struct FlushMember {
        template<typename T>
        constexpr Result<void> operator()(T& writer) const
        requires(requires {
                     { writer.flush() } -> concepts::ImplicitlyConvertibleTo<Result<void>>;
                 })
        {
            return writer.flush();
        }
    };

}

struct WriteSome : Dispatcher<WriteSome, Result<usize>(This&, Span<Byte const>), detail::WriteSomeMember> {};
struct Flush : Dispatcher<Flush, Result<void>(This&), detail::FlushMember> {};

constexpr inline auto write_some = WriteSome {};
constexpr inline auto flush = Flush {};

using Writer = meta::List<WriteSome, Flush>;
}