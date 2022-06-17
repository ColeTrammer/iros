#pragma once

#include <liim/forward.h>
#include <liim/utilities.h>

namespace LIIM::Error::Detail {
template<typename T, typename E>
struct ResultFor {
    using Type = Result<T, E>;
};

template<typename E>
struct ResultFor<void, E> {
    using Type = Result<void, E>;
};

template<typename T, typename U>
struct CommonErrorType {
    using Type = Variant<T, U>;
};

template<typename T>
struct CommonErrorType<T, T> {
    using Type = T;
};

template<typename T, typename TD, typename U, typename UD>
struct CommonErrorType<Error<T, TD>, Error<U, UD>> {
    using Type = Error<>;
};

template<typename ReturnValue, typename Result, int result_flags>
struct CommonResultImpl;

template<typename ReturnValue, typename Result>
struct CommonResultImpl<ReturnValue, Result, 0 /* Neither are results */> {
    using Type = ReturnValue;
};

template<typename ReturnValue, typename Result>
struct CommonResultImpl<ReturnValue, Result, 1 /* Result is a result */> {
    using Type = ResultFor<ReturnValue, typename Result::ErrorType>::Type;
};

template<typename ReturnValue, typename Result>
struct CommonResultImpl<ReturnValue, Result, 2 /* ReturnValue is a result */> {
    using Type = ReturnValue;
};

template<typename ReturnValue, typename Result>
struct CommonResultImpl<ReturnValue, Result, 3 /* Both are results */> {
    using Error = CommonErrorType<typename ReturnValue::ErrorType, typename Result::ErrorType>::Type;
    using Type = ResultFor<typename ReturnValue::ValueType, Error>::Type;
};

template<typename ReturnValue, typename... Results>
struct CommonResultHelper;

template<typename ReturnValue, typename Result, typename... Results>
struct CommonResultHelper<ReturnValue, Result, Results...> {
    using NextReturnType = CommonResultImpl<ReturnValue, Result, (IsResult<ReturnValue> << 1) | IsResult<Result>>::Type;
    using Type = CommonResultHelper<NextReturnType, Results...>::Type;
};

template<typename ReturnValue>
struct CommonResultHelper<ReturnValue> {
    using Type = ReturnValue;
};
}

namespace LIIM::Error {
template<typename ReturnValue, typename... Results>
using CommonResult = Detail::CommonResultHelper<ReturnValue, decay_t<Results>...>::Type;
}

using LIIM::Error::CommonResult;