#pragma once

#include <di/types/size_t.h>
#include <di/util/get.h>
#include <di/util/move.h>
#include <di/vocab/tuple/enable_generate_structed_bindings.h>

namespace di::meta {
template<typename Self>
struct AddMemberGet {
private:
    constexpr Self& self() & { return static_cast<Self&>(*this); }
    constexpr Self const& self() const& { return static_cast<Self const&>(*this); }
    constexpr Self&& self() && { return static_cast<Self&&>(*this); }
    constexpr Self const&& self() const&& { return static_cast<Self const&&>(*this); }

public:
    template<types::size_t index>
    requires(requires(Self& self) { util::get<index>(self); })
    constexpr decltype(auto) get() & {
        return util::get<index>(self());
    }

    template<types::size_t index>
    requires(requires(Self const& self) { util::get<index>(self); })
    constexpr decltype(auto) get() const& {
        return util::get<index>(self());
    }

    template<types::size_t index>
    requires(requires(Self&& self) { util::get<index>(util::move(self)); })
    constexpr decltype(auto) get() && {
        return util::get<index>(util::move(*this).self());
    }

    template<types::size_t index>
    requires(requires(Self const&& self) { util::get<index>(util::move(self)); })
    constexpr decltype(auto) get() const&& {
        return util::get<index>(util::move(*this).self());
    }

    template<typename T>
    requires(requires(Self& self) { util::get<T>(self); })
    constexpr decltype(auto) get() & {
        return util::get<T>(self());
    }

    template<typename T>
    requires(requires(Self const& self) { util::get<T>(self); })
    constexpr decltype(auto) get() const& {
        return util::get<T>(self());
    }

    template<typename T>
    requires(requires(Self&& self) { util::get<T>(util::move(self)); })
    constexpr decltype(auto) get() && {
        return util::get<T>(util::move(*this).self());
    }

    template<typename T>
    requires(requires(Self const&& self) { util::get<T>(util::move(self)); })
    constexpr decltype(auto) get() const&& {
        return util::get<T>(util::move(*this).self());
    }

private:
    constexpr friend bool tag_invoke(types::Tag<vocab::enable_generate_structed_bindings>, types::InPlaceType<Self>) {
        return true;
    }
};
}
