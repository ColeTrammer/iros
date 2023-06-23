#pragma once

#include <di/concepts/enum.h>
#include <di/concepts/unsigned_integral.h>
#include <di/meta/core.h>
#include <di/meta/remove_cv.h>
#include <di/meta/underlying_type.h>
#include <di/types/integers.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct MakeUnsignedHelper {};

    template<concepts::Enum T>
    struct MakeUnsignedHelper<T> : MakeUnsignedHelper<UnderlyingType<T>> {};

    template<concepts::UnsignedIntegral T>
    struct MakeUnsignedHelper<T> : TypeConstant<T> {};

    template<>
    struct MakeUnsignedHelper<char> : TypeConstant<unsigned char> {};

    template<>
    struct MakeUnsignedHelper<signed char> : TypeConstant<unsigned char> {};

    template<>
    struct MakeUnsignedHelper<short> : TypeConstant<unsigned short> {};

    template<>
    struct MakeUnsignedHelper<int> : TypeConstant<unsigned int> {};

    template<>
    struct MakeUnsignedHelper<long> : TypeConstant<unsigned long> {};

    template<>
    struct MakeUnsignedHelper<long long> : TypeConstant<unsigned long long> {};

#ifdef DI_HAVE_128_BIT_INTEGERS
    template<>
    struct MakeUnsignedHelper<types::i128> : TypeConstant<types::u128> {};
#endif

    template<>
    struct MakeUnsignedHelper<char8_t> : TypeConstant<types::u8> {};

    template<>
    struct MakeUnsignedHelper<char16_t> : TypeConstant<types::u16> {};

    template<>
    struct MakeUnsignedHelper<char32_t> : TypeConstant<types::u32> {};
}

template<typename T>
using MakeUnsigned = detail::MakeUnsignedHelper<RemoveCV<T>>::Type;
}
