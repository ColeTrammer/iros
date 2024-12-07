#pragma once

#include "di/container/concepts/forward_container.h"
#include "di/container/path/constant_path_interface.h"
#include "di/container/path/path_view_impl.h"
#include "di/container/string/string_impl.h"
#include "di/container/view/concat.h"
#include "di/meta/util.h"
#include "di/vocab/expected/as_fallible.h"
#include "di/vocab/optional/prelude.h"

namespace di::container {
template<concepts::InstanceOf<string::StringImpl> Str>
class PathImpl : public ConstantPathInterface<PathImpl<Str>, meta::Encoding<Str>> {
private:
    using Enc = meta::Encoding<Str>;

    template<concepts::ContainerCompatible<meta::EncodingCodeUnit<Enc>> Con, typename... Args>
    requires(concepts::CreatableFrom<Str, Con, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<PathImpl>, Con&& container,
                                     Args&&... args) {
        return as_fallible(util::create<Str>(util::forward<Con>(container), util::forward<Args>(args)...)) %
                   [](Str&& string) {
                       return PathImpl(util::move(string));
                   } |
               try_infallible;
    }

    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<PathImpl>, PathViewImpl<Enc> view) {
        return as_fallible(util::create<Str>(view.data())) % [](Str&& string) {
            return PathImpl(util::move(string));
        } | try_infallible;
    }

public:
    using Encoding = Enc;

    PathImpl() = default;

    constexpr PathImpl(Str&& string) : m_data(util::move(string)) { this->compute_first_component_end(); }

    constexpr auto data() const { return m_data.view(); }

    constexpr auto c_str() const
    requires(string::encoding::NullTerminated<Enc>)
    {
        return m_data.c_str();
    }

    constexpr auto clone() const {
        return as_fallible(util::clone(m_data)) % [](Str&& string) {
            return PathImpl(util::move(string));
        } | try_infallible;
    }

    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc> && concepts::ForwardContainer<Con>)
    constexpr auto append(Con&& container) -> decltype(auto) {
        using CodePoint = meta::EncodingCodePoint<Enc>;

        if (!container::empty(container)) {
            auto first_code_point = *container::begin(container);
            if (first_code_point == CodePoint('/')) {
                m_data.clear();
            }
        }

        if (!m_data.empty() && !m_data.ends_with(CodePoint('/'))) {
            return invoke_as_fallible([&] {
                       return m_data.push_back(CodePoint('/'));
                   }) >>
                       [&] {
                           return as_fallible(m_data.append(util::forward<Con>(container))) % [&](auto&&) {
                               return util::ref(*this);
                           };
                       } |
                   if_error([&](auto&&) {
                       m_data.pop_back();
                   }) |
                   try_infallible;
        }

        return as_fallible(m_data.append(util::forward<Con>(container))) % [&](auto&&) {
            return util::ref(*this);
        } | try_infallible;
    }
    constexpr auto append(PathViewImpl<Enc> view) -> decltype(auto) { return append(view.data()); }
    constexpr auto append(PathImpl const& other) -> decltype(auto) { return append(other.data()); }

    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc> && concepts::ForwardContainer<Con>)
    constexpr auto operator/=(Con&& container) -> decltype(auto) {
        return append(util::forward<Con>(container));
    }
    constexpr auto operator/=(PathViewImpl<Enc> view) -> decltype(auto) { return append(view.data()); }
    constexpr auto operator/=(PathImpl const& other) -> decltype(auto) { return append(other.data()); }

    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr auto concat(Con&& container) -> decltype(auto) {
        return m_data.append(util::forward<Con>(container)) % [&](auto&&) {
            return util::ref(*this);
        };
    }
    constexpr auto concat(PathViewImpl<Enc> view) -> decltype(auto) { return concat(view.data()); }
    constexpr auto concat(PathImpl const& other) -> decltype(auto) { return concat(other.data()); }

    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr auto operator+=(Con&& container) -> decltype(auto) {
        return concat(util::forward<Con>(container));
    }
    constexpr auto operator+=(PathViewImpl<Enc> view) -> decltype(auto) { return concat(view.data()); }
    constexpr auto operator+=(PathImpl const& other) -> decltype(auto) { return concat(other.data()); }

    constexpr void clear() { m_data.clear(); }

    constexpr auto take_underlying_string() && { return util::move(m_data); }

private:
    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr friend auto operator/(PathImpl&& lhs, Con&& rhs) {
        return as_fallible(lhs.append(util::forward<Con>(rhs))) % [&](auto&&) {
            return util::move(lhs);
        } | try_infallible;
    }
    friend auto operator/(PathImpl&& lhs, PathViewImpl<Enc> rhs) {
        return as_fallible(lhs.append(rhs.data())) % [&](auto&&) {
            return util::move(lhs);
        } | try_infallible;
    }
    friend auto operator/(PathImpl&& lhs, PathImpl const& rhs) { return lhs / rhs.data(); }

    Str m_data;
};
}
