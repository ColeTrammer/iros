#pragma once

#include <di/concepts/default_constructible.h>

namespace di::concepts {
template<typename T>
concept DefaultInitializable = DefaultConstructible<T> && requires {
                                                              T();
                                                              T {};
                                                              (void) ::new T;
                                                          };
}
