#include <di/container/string/prelude.h>
#include <di/container/view/prelude.h>
#include <di/parser/prelude.h>
#include <stdlib.h>

namespace di::parser {
namespace detail {
    struct AsC32Function {
        constexpr c32 operator()(char value) const { return c32(value); }
    };
}

constexpr inline auto as_c32 = detail::AsC32Function {};

class ZStringParserContext {
private:
    using View = container::string::StringViewImpl<container::string::TransparentEncoding>;
    using CodePoints = decltype(util::declval<container::ZCString&>() | di::transform(as_c32));
    using Iter = meta::ContainerIterator<CodePoints>;

public:
    constexpr explicit ZStringParserContext(container::ZCString string)
        : m_code_points(string | di::transform(as_c32)), m_iterator(container::begin(m_code_points)) {}

    using Error = vocab::Error;
    using Encoding = container::string::TransparentEncoding;

    constexpr auto begin() const { return m_iterator; }
    constexpr auto end() const { return container::end(m_code_points); }

    constexpr auto encoding() const { return m_encoding; }
    constexpr auto advance(Iter it) { m_iterator = it; }
    constexpr auto make_error() { return Error { BasicError::Invalid }; }

private:
    constexpr friend auto tag_invoke(types::Tag<reconstruct>, InPlaceType<ZStringParserContext>, Iter iter, Iter sent) {
        return reconstruct(in_place_type<View>, iter.base(), sent.base());
    }

    CodePoints m_code_points;
    Iter m_iterator;
    [[no_unique_address]] Encoding m_encoding;
};
}

namespace di::parser::detail {
constexpr auto tag_invoke(types::Tag<parser::into_parser_context>, ZCString value) {
    return parser::ZStringParserContext { value };
}
}

extern "C" int atoi(char const* string) {
    return di::run_parser_partial(
               ~di::parser::match_zero_or_more(' '_m || '\f'_m || '\n'_m || '\r'_m || '\t'_m || '\v'_m) >>
                   di::parser::integer<int>(),
               di::ZCString(string))
        .value_or(0);
}
