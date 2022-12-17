#pragma once

#include <di/container/string/encoding.h>

namespace di::container::string {
template<concepts::Encoding Enc>
class StringViewImpl;
}

namespace di::parser {
template<concepts::Encoding Enc>
class StringViewParserContext {
private:
    using View = container::string::StringViewImpl<Enc>;
    using CodePoints = decltype(util::declval<View const&>().unicode_code_points());
    using Iter = meta::ContainerIterator<CodePoints>;

public:
    constexpr explicit StringViewParserContext(View view)
        : m_code_points(view.unicode_code_points()), m_iterator(container::begin(m_code_points)) {}

    struct Error {};

    constexpr auto begin() const { return m_iterator; }
    constexpr auto end() const { return container::end(m_code_points); }

    constexpr auto advance(Iter it) { m_iterator = it; }
    constexpr auto make_error() { return Error {}; }

private:
    template<typename Iter, typename Sent>
    requires(concepts::ReconstructibleContainer<View, Iter, Sent>)
    constexpr friend auto tag_invoke(types::Tag<reconstruct>, InPlaceType<StringViewParserContext>, Iter&& iter, Sent&& sent) {
        return reconstruct(in_place_type<View>, util::forward<Iter>(iter), util::forward<Sent>(sent));
    }

    CodePoints m_code_points;
    Iter m_iterator;
};
}