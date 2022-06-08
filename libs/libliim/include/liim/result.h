#pragma once

#include <liim/format/builtin_formatters.h>
#include <liim/option.h>
#include <liim/variant.h>

namespace LIIM {
template<typename T>
struct Ok {
    T ok;

    constexpr T&& try_move_out() { return move(ok); }
    constexpr T try_move_out() requires(IsLValueReference<T>::value) { return ok; }
};

template<typename T>
Ok(T) -> Ok<T>;

template<typename E>
struct Err {
    E err;
};

template<typename E>
Err(E) -> Err<E>;

template<typename T, typename E>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;

    constexpr Result(Ok<T>&& value) : m_impl(in_place_index<0>, move(value.ok)) {}
    constexpr Result(Err<E>&& error) : m_impl(in_place_index<1>, move(error.err)) {}

    template<typename U>
    constexpr Result(Ok<U>&& value) : m_impl(in_place_index<0>, move(value.ok)) {}

    template<typename F>
    constexpr Result(Err<F>&& error) : m_impl(in_place_index<1>, move(error.err)) {}

    template<typename U, typename F>
    constexpr Result(Result<U, F>&& other) {
        if (other.is_ok()) {
            m_impl.emplace(in_place_index<0>, move(other.value()));
        } else {
            m_impl.emplace(in_place_index<1>, move(other.error()));
        }
    }

    template<typename U, typename F>
    friend class Result;

    constexpr bool operator==(const Result& other) const { return this->m_impl == other.m_impl; }
    constexpr bool operator!=(const Result& other) const { return this->m_impl != other.m_impl; }

    constexpr bool is_ok() const { return m_impl.template is<0>(); }
    constexpr bool is_error() const { return m_impl.template is<1>(); }

    constexpr decltype(auto) value() { return m_impl.template get<0>(); }
    constexpr decltype(auto) value() const { return m_impl.template get<0>(); }

    constexpr decltype(auto) error() { return m_impl.template get<1>(); }
    constexpr decltype(auto) error() const { return m_impl.template get<1>(); }

    constexpr void swap(Result& other) { this->m_impl.swap(other.m_impl); }

    constexpr T value_or(T other) {
        if (is_ok()) {
            return move(value());
        }
        return other;
    }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr Result<R, E> map(C mapper) && {
        if (!is_ok()) {
            return Err(forward<E&&>(error()));
        }
        return Ok(mapper(forward<T&&>(value())));
    }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr Result<R, E> map(C mapper) const& {
        if (!is_ok()) {
            return Err(error());
        }
        return Ok(mapper(value()));
    }

    template<typename C, typename R = InvokeResult<C, E>::type>
    constexpr Result<T, R> map_error(C mapper) && {
        if (!is_error()) {
            return Ok(forward<T&&>(value()));
        }
        return Err(mapper(forward<E&&>(error())));
    }

    template<typename C, typename R = InvokeResult<C, E>::type>
    constexpr Result<T, R> map_error(C mapper) const& {
        if (!is_error()) {
            return Ok(value());
        }
        return Err(mapper(error()));
    }

    constexpr operator bool() const { return is_ok(); }
    constexpr bool operator!() const { return !is_ok(); }

    constexpr Err<E> try_did_fail() {
        assert(is_error());
        if constexpr (IsLValueReference<E>::value) {
            return Err(error());
        } else {
            return Err(move(error()));
        }
    }

    constexpr Ok<T> try_did_succeed() {
        assert(is_ok());
        return Ok(move(value()));
    }

    constexpr Ok<T> try_did_succeed() requires(IsLValueReference<T>::value) {
        assert(is_ok());
        return Ok<T>(value());
    }

private : Variant<T, E> m_impl;
};
}

namespace LIIM::Format {
template<Formattable T, Formattable E>
struct Formatter<Result<T, E>> : public BaseFormatter {
    void format(const Result<T, E>& value, FormatContext& context) {
        if (value.is_error()) {
            return format_to_context(context, "Err({})", value.error());
        }
        return format_to_context(context, "Ok({})", value.value());
    }
};
}

using LIIM::Err;
using LIIM::Ok;
using LIIM::Result;
