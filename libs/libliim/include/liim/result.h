#pragma once

#include <liim/format/builtin_formatters.h>
#include <liim/option.h>
#include <liim/variant.h>

namespace LIIM {
template<typename T>
struct Ok {
    T ok;
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
    constexpr Result(Ok<T>&& value) : m_impl(in_place_index<0>, move(value.ok)) {}
    constexpr Result(Err<E>&& error) : m_impl(in_place_index<1>, move(error.err)) {}

    constexpr bool operator==(const Result& other) const { return this->m_impl == other.m_impl; }
    constexpr bool operator!=(const Result& other) const { return this->m_impl != other.m_impl; }

    constexpr bool is_ok() const { return m_impl.template is<0>(); }
    constexpr bool is_error() const { return m_impl.template is<1>(); }

    constexpr T& value() { return m_impl.template get<0>(); }
    constexpr const T& value() const { return m_impl.template get<0>(); }

    constexpr E& error() { return m_impl.template get<1>(); }
    constexpr const E& error() const { return m_impl.template get<1>(); }

    constexpr void swap(Result& other) { this->m_impl.swap(other.m_impl); }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr Result<R, E> map(C mapper) const {
        if (!is_ok()) {
            return Err(move(error()));
        }
        return Ok(mapper(move(value())));
    }

    template<typename C, typename R = InvokeResult<C, E>::type>
    constexpr Result<T, R> map_error(C mapper) const {
        if (!is_error()) {
            return Ok(move(value()));
        }
        return Err(mapper(move(error())));
    }

    constexpr operator bool() const { return is_ok(); }
    constexpr bool operator!() const { return !is_ok(); }

    constexpr Err<E> try_did_fail() {
        assert(is_error());
        return Err(move(error()));
    }

private:
    Variant<T, E> m_impl;
};
}

namespace LIIM::Format {
template<typename T, typename E>
struct Formatter<Result<T, E>> : public BaseFormatter {
    void format(const Result<T, E>& value, FormatContext& context) {
        if (value.is_error()) {
            return format_string_view(Format::format("Error({})", value.error()).view(), context);
        }
        return format_string_view(Format::format("Ok({})", value.value()).view(), context);
    }
};
}

using LIIM::Err;
using LIIM::Ok;
using LIIM::Result;
