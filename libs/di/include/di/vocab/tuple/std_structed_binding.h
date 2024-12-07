#pragma once

#include "di/types/size_t.h"

#ifndef DI_NO_USE_STD
#include <utility>
#else
namespace std {
template<typename T>
struct tuple_size;

template<di::types::size_t index, typename T>
struct tuple_element;
}
#endif
