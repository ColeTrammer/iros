#pragma once

#include "di/any/prelude.h"
#include "di/vocab/error/prelude.h"
#include "di/vocab/span/prelude.h"

namespace di::io {
namespace detail {
    struct ReadSomeMember {
        template<typename T>
        constexpr auto operator()(T& reader, Span<Byte> data) const -> Result<usize>
        requires(requires {
            { reader.read_some(data) } -> concepts::ImplicitlyConvertibleTo<Result<usize>>;
        })
        {
            return reader.read_some(data);
        }

        template<typename T>
        constexpr auto operator()(util::ReferenceWrapper<T> reader, Span<Byte> data) const -> Result<usize>
        requires(requires {
            { (*this)(reader.get(), data) };
        })
        {
            return (*this)(reader.get(), data);
        }
    };
}

struct ReadSome : Dispatcher<ReadSome, Result<usize>(This&, Span<Byte>), detail::ReadSomeMember> {};

constexpr inline auto read_some = ReadSome {};

using Reader = meta::List<ReadSome>;
}

namespace di::meta {
template<typename T, concepts::Impl<io::Reader> Reader>
using ReaderResult =
    meta::LikeExpected<decltype(io::read_some(util::declval<Reader&>(), util::declval<Span<Byte>>())), T>;
}

namespace di {
using io::read_some;
using io::Reader;
}
