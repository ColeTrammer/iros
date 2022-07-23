#pragma once

#include <liim/erased.h>
#include <liim/error/forward.h>
#include <liim/utilities.h>

namespace LIIM::Error {
template<typename T>
concept Erasable = LIIM::Erasable<T, sizeof(void*), alignof(void*)>;

template<typename T = void>
using ErrorTransport = ErasedStorage<T, sizeof(void*), alignof(void*)>;

template<Erasable T>
ErrorTransport<T> error_transport_cast(ErrorTransport<> value) {
    return erased_storage_cast<T>(value);
}
}
