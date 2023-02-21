#pragma once

#include <di/concepts/decays_to.h>
#include <di/container/path/constant_path_interface.h>
#include <di/container/path/path_view_impl.h>
#include <di/container/string/string_impl.h>
#include <di/container/view/concat.h>
#include <di/vocab/optional/prelude.h>

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

    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr decltype(auto) append(Con&& container) {
        using CodePoint = meta::EncodingCodePoint<Enc>;

        if (this->is_absolute()) {
            m_data.clear();
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
                       // FIXME: uncomment once String has pop_back().
                       //    m_data.pop_back();
                   }) |
                   try_infallible;
        }

        return as_fallible(m_data.append(util::forward<Con>(container))) % [&](auto&&) {
            return util::ref(*this);
        } | try_infallible;
    }
    constexpr decltype(auto) append(PathViewImpl<Enc> view) { return append(view.data()); }
    constexpr decltype(auto) append(PathImpl const& other) { return append(other.data()); }

    template<concepts::ContainerCompatible<meta::EncodingCodePoint<Enc>> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr decltype(auto) concat(Con&& container) {
        return m_data.append(util::forward<Con>(container)) % [&](auto&&) {
            return util::ref(*this);
        };
    }
    constexpr decltype(auto) concat(PathViewImpl<Enc> view) { return concat(view.data()); }
    constexpr decltype(auto) concat(PathImpl const& other) { return concat(other.data()); }

    constexpr void clear() { m_data.clear(); }

private:
    Str m_data;
};
}
