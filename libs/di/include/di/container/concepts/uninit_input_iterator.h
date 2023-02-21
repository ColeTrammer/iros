#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/concepts/same_as.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_value.h>
#include <di/meta/remove_cvref.h>

namespace di::concepts {
template<typename T>
concept UninitInputIterator = InputIterator<T> && LValueReference<meta::IteratorReference<T>> &&
                              SameAs<meta::RemoveCVRef<meta::IteratorReference<T>>, meta::IteratorValue<T>>;
}
