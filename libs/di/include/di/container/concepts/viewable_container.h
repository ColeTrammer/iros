#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/initializer_list.h>
#include <di/concepts/lvalue_reference.h>
#include <di/concepts/movable.h>
#include <di/container/concepts/container.h>
#include <di/container/concepts/view.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>

namespace di::concepts {
// Any View is considered viewable, if and only if the View can be constructed from T.
// Non-views can be viewable, if they are either an lvalue, or they are movable (into the view), with
// the exception of std::initializer_list, which does not own its underlying values.
template<typename T>
concept ViewableContainer = Container<T> &&
                            ((View<meta::RemoveCVRef<T>> && ConstructibleFrom<meta::RemoveCVRef<T>, T>) ||
                             (!View<meta::RemoveCVRef<T>> &&
                              (LValueReference<T> || (Movable<meta::RemoveReference<T>> && !InitializerList<T>) )));
}
