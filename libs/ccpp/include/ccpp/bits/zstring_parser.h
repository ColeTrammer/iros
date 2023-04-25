#pragma once

#include <di/container/meta/container_iterator.h>
#include <di/container/string/zstring.h>
#include <di/parser/basic/integer.h>
#include <di/parser/prelude.h>
#include <di/types/prelude.h>

namespace di::parser {
namespace detail {
    struct AsC32Function {
        constexpr c32 operator()(char value) const { return c32(value); }
    };
}

constexpr inline auto as_c32 = detail::AsC32Function {};

enum class ZStringError {
    Invalid,
    Overflow,
    Underflow,
};

class ZStringParserContext {
private:
    using View = container::string::StringViewImpl<container::string::TransparentEncoding>;
    using CodePoints = decltype(util::declval<container::ZCString&>() | di::transform(as_c32));
    using ZIter = meta::ContainerIterator<container::ZCString>;
    using Iter = meta::ContainerIterator<CodePoints>;

public:
    constexpr explicit ZStringParserContext(container::ZCString string)
        : m_code_points(string | di::transform(as_c32)), m_iterator(container::begin(m_code_points)) {}

    using Error = ZStringError;
    using Encoding = container::string::TransparentEncoding;

    constexpr auto begin() const { return m_iterator; }
    constexpr auto end() const { return container::end(m_code_points); }

    constexpr auto encoding() const { return m_encoding; }
    constexpr auto advance(Iter it) { m_iterator = it; }
    constexpr auto make_error() { return ZStringError::Invalid; }
    constexpr auto make_error(IntegerError error, ZIter iterator_on_error) {
        m_iterator_on_error = iterator_on_error;
        return error == IntegerError::Overflow ? ZStringError::Overflow : ZStringError::Underflow;
    }

    constexpr auto iterator_on_error() const { return m_iterator_on_error; }

private:
    constexpr friend auto tag_invoke(types::Tag<reconstruct>, InPlaceType<ZStringParserContext>, Iter iter, Iter sent) {
        return reconstruct(in_place_type<View>, iter.base(), sent.base());
    }

    CodePoints m_code_points;
    Iter m_iterator;
    ZIter m_iterator_on_error;
    [[no_unique_address]] Encoding m_encoding;
};
}

namespace di::parser::detail {
constexpr auto tag_invoke(types::Tag<parser::into_parser_context>, ZCString value) {
    return parser::ZStringParserContext { value };
}
}
