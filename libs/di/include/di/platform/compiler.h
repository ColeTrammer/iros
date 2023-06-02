#pragma once

#ifdef __clang__
#define DI_CLANG 1
#elif defined(__GNUC__)
#define DI_GCC 1
#elif defined(_MSC_VER)
#define DI_MSVC 1
#endif

#ifdef DI_GCC
// NOTE: GCC's [[no_unique_address]] is broken with immovable types...
#define DI_IMMOVABLE_NO_UNIQUE_ADDRESS
#else
#define DI_IMMOVABLE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
