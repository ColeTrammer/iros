#pragma once

#include <di/container/algorithm/all_of.h>
#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/path/constant_path_interface.h>
#include <di/container/path/path_iterator.h>
#include <di/container/string/string_view.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::container {
template<concepts::Encoding Enc>
class PathViewImpl
    : public meta::EnableView<PathViewImpl<Enc>>
    , public meta::EnableBorrowedContainer<PathViewImpl<Enc>>
    , public ConstantPathInterface<PathViewImpl<Enc>, Enc> {
private:
    using View = string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using ViewIter = meta::ContainerIterator<View>;

public:
    PathViewImpl() = default;

    constexpr PathViewImpl(View view) : m_view(view) { this->compute_first_component_end(); }

    constexpr View data() const { return m_view; }

private:
    View m_view;
};
}