#pragma once

#include <di/any/concepts/impl.h>
#include <di/assert/assert_bool.h>
#include <di/container/string/encoding.h>
#include <di/container/string/transparent_encoding.h>
#include <di/container/string/utf8_encoding.h>
#include <di/io/interface/writer.h>

namespace di::io {
namespace detail {
    struct WriteExactlyFunction {
        template<concepts::Impl<Writer> Writer>
        constexpr auto operator()(Writer& writer, vocab::Span<byte const> data) const
            -> meta::WriterResult<void, Writer> {
            auto nwritten = usize { 0 };
            while (nwritten < data.size()) {
                auto n = DI_TRY(write_some(writer, *data.subspan(nwritten)));
                DI_ASSERT(n > 0);
                nwritten += n;
            }
            return {};
        };

        template<concepts::Impl<Writer> Writer>
        constexpr auto operator()(Writer& writer, char data) const -> meta::WriterResult<void, Writer> {
            auto byte = di::byte(data);
            return (*this)(writer, vocab::Span { &byte, 1 });
        };

        template<concepts::Impl<Writer> Writer, concepts::detail::ConstantString T>
        requires(concepts::SameAs<meta::Encoding<T>, container::string::Utf8Encoding>)
        constexpr auto operator()(Writer& writer, T const& data) const -> meta::WriterResult<void, Writer> {
            for (auto ch : data) {
                DI_TRY((*this)(writer, char(ch)));
            }
            return {};
        };

        template<concepts::Impl<Writer> Writer, concepts::detail::ConstantString T>
        requires(concepts::SameAs<meta::Encoding<T>, container::string::TransparentEncoding>)
        constexpr auto operator()(Writer& writer, T const& data) const -> meta::WriterResult<void, Writer> {
            for (auto ch : data) {
                DI_TRY((*this)(writer, ch));
            }
            return {};
        };
    };
}

constexpr inline auto write_exactly = detail::WriteExactlyFunction {};
}

namespace di {
using io::write_exactly;
}
