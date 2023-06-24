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
    constexpr static T* begin() { return nullptr; }
    constexpr static T* end() { return nullptr; }
    constexpr static T* data() { return nullptr; }
    constexpr static types::size_t size() { return 0; }
    constexpr static bool empty() { return true; }

private:
    constexpr friend EmptyView tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, T*, T*) {
        return EmptyView {};
    }
    constexpr friend EmptyView tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, nullptr_t, T*) {
        return EmptyView {};
    }
    constexpr friend EmptyView tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, T*, nullptr_t) {
        return EmptyView {};
    }
    constexpr friend EmptyView tag_invoke(types::Tag<container::reconstruct>, InPlaceType<EmptyView>, nullptr_t,
                                          nullptr_t) {
        return EmptyView {};
    }
};
}
