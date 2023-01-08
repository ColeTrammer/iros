#pragma once

#include <di/concepts/enum.h>
#include <di/concepts/signed_integral.h>
#include <di/meta/remove_cv.h>
#include <di/meta/type_constant.h>
#include <di/meta/underlying_type.h>
#include <di/types/integers.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct MakeSignedHelper {};

    template<concepts::Enum T>
    struct MakeSignedHelper<T> : MakeSignedHelper<UnderlyingType<T>> {};

    template<concepts::SignedIntegral T>
    struct MakeSignedHelper<T> : TypeConstant<T> {};

    template<>
    struct MakeSignedHelper<unsigned char> : TypeConstant<char> {};

    template<>
    struct MakeSignedHelper<unsigned short> : TypeConstant<short> {};

    template<>
    struct MakeSignedHelper<unsigned int> : TypeConstant<int> {};

    template<>
    struct MakeSignedHelper<unsigned long> : TypeConstant<long> {};

    template<>
    struct MakeSignedHelper<unsigned long long> : TypeConstant<long long> {};

#ifdef DI_HAVE_128_BIT_INTEGERS
    template<>
    struct MakeSignedHelper<types::u128> : TypeConstant<types::i128> {};
#endif

    template<>
    struct MakeSignedHelper<char8_t> : TypeConstant<types::i8> {};

    template<>
    struct MakeSignedHelper<char16_t> : TypeConstant<types::i16> {};

    template<>
    struct MakeSignedHelper<char32_t> : TypeConstant<types::i32> {};
}

template<typename T>
using MakeSigned = detail::MakeSignedHelper<RemoveCV<T>>::Type;
}
