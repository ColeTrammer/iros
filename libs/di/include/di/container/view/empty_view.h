#pragma once

#include <di/container/interface/reconstruct.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/view/view_interface.h>
#include <di/meta/language.h>
#include <di/types/size_t.h>

namespace di::container {
template<typename T>
requires(concepts::Object<T>)
class EmptyView
    : public ViewInterface<EmptyView<T>>
    , public meta::EnableBorrowedContainer<EmptyView<T>> {
public:
    constexpr static auto begin() -> T* { return nullptr; }
    constexpr static auto end() -> T* { return nullptr; }
    constexpr static auto data() -> T* { return nullptr; }
    constexpr static auto size() -> types::size_t { return 0; }
    constexpr static auto empty() -> bool { return true; }

private:
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, T*, T*) -> EmptyView {
        return EmptyView {};
    }
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, nullptr_t, T*)
        -> EmptyView {
        return EmptyView {};
    }
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, T*, nullptr_t)
        -> EmptyView {
        return EmptyView {};
    }
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, nullptr_t, nullptr_t)
        -> EmptyView {
        return EmptyView {};
    }
};
}
