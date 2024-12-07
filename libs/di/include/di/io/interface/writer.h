#pragma once

#include "di/any/prelude.h"
#include "di/meta/vocab.h"
#include "di/util/reference_wrapper.h"
#include "di/vocab/error/prelude.h"
#include "di/vocab/span/prelude.h"

namespace di::io {
namespace detail {
    struct WriteSomeMember {
        template<typename T>
        constexpr auto operator()(T& writer, Span<Byte const> data) const -> Result<usize>
        requires(requires {
            { writer.write_some(data) } -> concepts::ImplicitlyConvertibleTo<Result<usize>>;
        })
        {
            return writer.write_some(data);
        }

        template<typename T>
        constexpr auto operator()(util::ReferenceWrapper<T> writer, Span<Byte const> data) const -> Result<usize>
        requires(requires {
            { (*this)(writer.get(), data) };
        })
        {
            return (*this)(writer.get(), data);
        }
    };

    struct FlushMember {
        template<typename T>
        constexpr auto operator()(T& writer) const -> Result<void>
        requires(requires {
            { writer.flush() } -> concepts::ImplicitlyConvertibleTo<Result<void>>;
        })
        {
            return writer.flush();
        }

        template<typename T>
        constexpr auto operator()(util::ReferenceWrapper<T> writer) const -> Result<void>
        requires(requires {
            { (*this)(writer.get()) };
        })
        {
            return (*this)(writer.get());
        }
    };

}

struct WriteSome : Dispatcher<WriteSome, Result<usize>(This&, Span<Byte const>), detail::WriteSomeMember> {};
struct Flush : Dispatcher<Flush, Result<void>(This&), detail::FlushMember> {};

constexpr inline auto write_some = WriteSome {};
constexpr inline auto flush = Flush {};

using Writer = meta::List<WriteSome, Flush>;
}

namespace di::meta {
template<typename T, concepts::Impl<io::Writer> Writer>
using WriterResult =
    meta::LikeExpected<decltype(io::write_some(util::declval<Writer&>(), util::declval<Span<Byte const>>())), T>;
}

namespace di {
using io::write_some;
using io::Writer;
}
