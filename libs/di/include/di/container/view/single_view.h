#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_initializable.h>
#include <di/concepts/movable.h>
#include <di/concepts/object.h>
#include <di/container/view/view_interface.h>
#include <di/types/in_place.h>
#include <di/types/size_t.h>
#include <di/util/address_of.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::container {
template<concepts::Movable T>
requires(concepts::Object<T>)
class SingleView : public ViewInterface<SingleView<T>> {
public:
    constexpr SingleView()
    requires(concepts::DefaultInitializable<T>)
    = default;

    constexpr explicit SingleView(T const& value)
    requires(concepts::CopyConstructible<T>)
        : m_value(value) {}

    constexpr explicit SingleView(T&& value) : m_value(util::move(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit SingleView(types::InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    constexpr T* begin() { return util::address_of(m_value); }
    constexpr T const* begin() const { return util::address_of(m_value); }

    constexpr auto end() { return begin() + 1; }
    constexpr auto end() const { return begin() + 1; }

    constexpr static types::size_t size() { return 1; }

    constexpr auto data() { return begin(); }
    constexpr auto data() const { return begin(); }

private:
    T m_value;
};

template<typename T>
SingleView(T) -> SingleView<T>;
}
