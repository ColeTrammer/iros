#pragma once

#include <di/concepts/decays_to.h>
#include <di/container/path/constant_path_interface.h>
#include <di/container/path/path_view_impl.h>
#include <di/container/string/string_impl.h>

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
    PathImpl() = default;

    constexpr PathImpl(Str&& string) : m_data(util::move(string)) { this->compute_first_component_end(); }

    constexpr auto data() const { return m_data.view(); }

    constexpr auto c_str() const
    requires(string::encoding::NullTerminated<Enc>)
    {
        return m_data.c_str();
    }

private:
    Str m_data;
};
}