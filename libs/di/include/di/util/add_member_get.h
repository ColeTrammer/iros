#pragma once

#include <di/types/size_t.h>
#include <di/util/get.h>
#include <di/util/move.h>
#include <di/vocab/tuple/enable_generate_structed_bindings.h>

namespace di::util {
template<typename Self>
struct AddMemberGet {
private:
    constexpr auto self() & -> Self& { return static_cast<Self&>(*this); }
    constexpr auto self() const& -> Self const& { return static_cast<Self const&>(*this); }
    constexpr auto self() && -> Self&& { return static_cast<Self&&>(*this); }
    constexpr auto self() const&& -> Self const&& { return static_cast<Self const&&>(*this); }

public:
    template<types::size_t index>
    requires(requires(Self& self) { util::get<index>(self); })
    constexpr auto get() & -> decltype(auto) {
        return util::get<index>(self());
    }

    template<types::size_t index>
    requires(requires(Self const& self) { util::get<index>(self); })
    constexpr auto get() const& -> decltype(auto) {
        return util::get<index>(self());
    }

    template<types::size_t index>
    requires(requires(Self&& self) { util::get<index>(util::move(self)); })
    constexpr auto get() && -> decltype(auto) {
        return util::get<index>(util::move(*this).self());
    }

    template<types::size_t index>
    requires(requires(Self const&& self) { util::get<index>(util::move(self)); })
    constexpr auto get() const&& -> decltype(auto) {
        return util::get<index>(util::move(*this).self());
    }

    template<typename T>
    requires(requires(Self& self) { util::get<T>(self); })
    constexpr auto get() & -> decltype(auto) {
        return util::get<T>(self());
    }

    template<typename T>
    requires(requires(Self const& self) { util::get<T>(self); })
    constexpr auto get() const& -> decltype(auto) {
        return util::get<T>(self());
    }

    template<typename T>
    requires(requires(Self&& self) { util::get<T>(util::move(self)); })
    constexpr auto get() && -> decltype(auto) {
        return util::get<T>(util::move(*this).self());
    }

    template<typename T>
    requires(requires(Self const&& self) { util::get<T>(util::move(self)); })
    constexpr auto get() const&& -> decltype(auto) {
        return util::get<T>(util::move(*this).self());
    }

private:
    constexpr friend auto tag_invoke(types::Tag<vocab::enable_generate_structed_bindings>, types::InPlaceType<Self>)
        -> bool {
        return true;
    }
};
}
