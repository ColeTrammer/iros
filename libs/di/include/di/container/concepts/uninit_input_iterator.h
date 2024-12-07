#pragma once

#include "di/container/concepts/input_iterator.h"
#include "di/container/meta/iterator_reference.h"
#include "di/container/meta/iterator_value.h"
#include "di/meta/core.h"
#include "di/meta/language.h"

namespace di::concepts {
template<typename T>
concept UninitInputIterator = InputIterator<T> && LValueReference<meta::IteratorReference<T>> &&
                              SameAs<meta::RemoveCVRef<meta::IteratorReference<T>>, meta::IteratorValue<T>>;
}
