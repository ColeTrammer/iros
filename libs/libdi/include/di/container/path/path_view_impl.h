#pragma once

#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/path/path_iterator.h>
#include <di/container/string/string_view.h>

namespace di::container {
template<concepts::Encoding Enc>
class PathViewImpl
    : public meta::EnableView<PathViewImpl<Enc>>
    , public meta::EnableBorrowedContainer<PathViewImpl<Enc>> {
private:
    using View = string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using ViewIter = meta::ContainerIterator<View>;

public:
    PathViewImpl() = default;

    constexpr PathViewImpl(View view) : m_view(view) {
        if (view.starts_with(CodePoint('/'))) {
            m_first_component_end = container::next(m_view.begin());
        } else {
            m_first_component_end = container::find(m_view, CodePoint('/'));
        }
    }

    constexpr auto begin() const { return PathIterator(m_view, { m_view.begin(), m_first_component_end }); }

    constexpr auto end() const { return PathIterator(m_view, { m_view.end(), m_view.end() }); }

private:
    constexpr friend bool operator==(PathViewImpl const& a, PathViewImpl const& b) { return container::equal(a, b); }
    constexpr friend auto operator<=>(PathViewImpl const& a, PathViewImpl const& b) { return container::compare(a, b); }

    View m_view;
    ViewIter m_first_component_end;
};
}