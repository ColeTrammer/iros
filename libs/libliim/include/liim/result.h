#pragma once

#include <liim/compare.h>
#include <liim/format/builtin_formatters.h>
#include <liim/option.h>
#include <liim/tuple.h>
#include <liim/utilities.h>

namespace LIIM {
template<typename E>
class Err {
public:
    constexpr Err(const Err&) = delete;
    constexpr Err(Err&&) = default;

    template<typename Er = E>
    requires(ConstructibleFrom<E, Er>) constexpr explicit Err(Er&& error) : m_error(forward<Er>(error)) {}

    constexpr const E& error() const& { return m_error; }
    constexpr E& error() & { return m_error; }
    constexpr const E&& error() const&& { return static_cast<const E&&>(m_error); }
    constexpr E&& error() && { return static_cast<E&&>(m_error); }

    template<EqualComparable<E> U>
    friend constexpr bool operator==(const Err& a, const Err<U>& b) {
        return a.error() == b.error();
    }

private:
    E m_error;
};

template<typename E>
Err(E) -> Err<E>;

namespace Detail {
    template<typename T>
    struct Wrapper {
        T&& try_move_out() && { return move(*this).value; }

        T value;
    };

    template<>
    struct Wrapper<void> {
        void try_move_out() {}
    };

    template<typename T>
    class UniformStorage {
    public:
        template<typename U>
        requires(ConstructibleFrom<T, U>) constexpr UniformStorage(U&& value) : m_value(forward<U>(value)) {}

        constexpr void destroy() { m_value.~T(); }

        constexpr T& value() & { return m_value; }
        constexpr const T& value() const& { return m_value; }
        constexpr T&& value() && { return move(m_value); }
        constexpr const T&& value() const&& { return move(m_value); }

        constexpr T* pointer() { return &m_value; }
        constexpr const T* pointer() const { return &m_value; }

        constexpr void swap(UniformStorage& other) { ::swap(this->value(), other.value()); }

    private:
        T m_value;
    };

    template<typename T>
    class UniformStorage<T&> {
    public:
        constexpr UniformStorage(T& value) : m_value(&value) {}

        constexpr void destroy() {}

        constexpr T& value() { return *m_value; }
        constexpr const T& value() const { return *m_value; }

        constexpr T* pointer() { return m_value; }
        constexpr const T* pointer() const { return m_value; }

        constexpr void swap(UniformStorage& other) { ::swap(this->m_value, other.m_value); }

    private:
        T* m_value { nullptr };
    };
}

template<typename T, typename E>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;
    using ErrType = Err<E>;

    constexpr Result(const Result&) = delete;

    constexpr Result(Result&& other) : m_has_value(other.has_value()) {
        if (has_value()) {
            construct_at(&m_value, move(other).value());
        } else {
            construct_at(&m_error, move(other).error());
        }
    }

    template<typename U>
    requires(ConstructibleFrom<T, U>) constexpr Result(U&& value) : m_has_value(true), m_value(forward<U>(value)) {}

    template<typename G>
    requires(ConstructibleFrom<E, G&&>) constexpr Result(Err<G>&& error) : m_has_value(false), m_error(move(error).error()) {}

    template<typename U, typename G>
    requires(ConstructibleFrom<T, U&&>&& ConstructibleFrom<E, G&&>) constexpr Result(Result<U, G>&& other)
        : m_has_value(other.has_value()) {
        if (has_value()) {
            construct_at(&m_value, move(other).value());
        } else {
            construct_at(&m_error, move(other).error());
        }
    }

    constexpr Result& operator=(Result&& other) {
        if (this != &other) {
            auto temp = Result(move(other));
            swap(temp);
        }
        return *this;
    }

    template<typename U>
    requires(ConstructibleFrom<T, U>) constexpr Result& operator=(U&& value) { return *this = Result(forward<U>(value)); }

    template<typename G>
    requires(ConstructibleFrom<E, G&&>) constexpr Result& operator=(Err<G>&& error) { return *this = Result(move(error)); }

    template<typename U, typename G>
    requires(ConstructibleFrom<T, U&&>&& ConstructibleFrom<E, G&&>) constexpr Result& operator=(Result<U, G>&& other) {
        return *this = Result(move(other));
    }

    constexpr ~Result() {
        if (has_value()) {
            m_value.destroy();
        } else {
            m_error.destroy();
        }
    }

    constexpr explicit operator bool() const { return has_value(); }
    constexpr bool has_value() const { return m_has_value; }
    constexpr bool is_error() const { return !has_value(); }

    constexpr auto operator->() const {
        assert(has_value());
        return m_value.pointer();
    }
    constexpr auto operator->() {
        assert(has_value());
        return m_value.pointer();
    }

    constexpr T& operator*() & {
        assert(has_value());
        return m_value.value();
    }
    constexpr const T& operator*() const& {
        assert(has_value());
        return m_value.value();
    }
    constexpr T&& operator*() && {
        assert(has_value());
        return move(m_value).value();
    }
    constexpr const T&& operator*() const&& {
        assert(has_value());
        return move(m_value).value();
    }

    constexpr T& value() & { return **this; }
    constexpr const T& value() const& { return **this; }
    constexpr T&& value() && { return *move(*this); }
    constexpr const T&& value() const&& { return *move(*this); }

    constexpr E& error() & {
        assert(is_error());
        return m_error.value();
    }
    constexpr const E& error() const& {
        assert(is_error());
        return m_error.value();
    }
    constexpr E&& error() && {
        assert(is_error());
        return move(m_error).value();
    }
    constexpr const E&& error() const&& {
        assert(is_error());
        return move(m_error).value();
    }

    template<typename U>
    requires(ConstructibleFrom<T, U>) constexpr T value_or(U&& other) && {
        if (has_value()) {
            return move(*this).value();
        } else {
            return forward<U>(other);
        }
    }

    template<EqualComparable<T> U, EqualComparable<E> G>
    friend constexpr bool operator==(const Result& a, const Result<U, G>& b) {
        if (a.has_value() && b.has_value()) {
            return a.value() == b.value();
        } else if (a.is_error() && b.is_error()) {
            return a.error() == b.error();
        } else {
            return false;
        }
    }

    template<typename U>
    requires(!IsResult<U> && EqualComparable<T, U>) friend constexpr bool operator==(const Result& a, const U& b) {
        return a.has_value() && a.value() == b;
    }

    template<typename G>
    requires(!IsResult<G> && EqualComparable<E, G>) friend constexpr bool operator==(const Result& a, const Err<G>& b) {
        return a.is_error() && a.error() == b.error();
    }

    template<typename F>
    constexpr Result<typename InvokeResult<F, T&&>::type, E> and_then(F&& f) && {
        if (has_value()) {
            return forward<F>(f)(move(*this).value());
        } else {
            return Err<E>(move(*this).error());
        }
    }

    template<typename F>
    constexpr Result<typename InvokeResult<F, T&&>::type, E> transform(F&& f) && {
        if (has_value()) {
            return forward<F>(f)(move(*this).value());
        } else {
            return Err<E>(move(*this).error());
        }
    }

    template<typename F>
    constexpr Result<T, typename InvokeResult<F, E&&>::type> transform_error(F&& f) && {
        if (has_value()) {
            return move(*this).value();
        } else {
            return Err<typename InvokeResult<F, E&&>::type>(forward<F>(f)(move(*this).error()));
        }
    }

    constexpr void swap(Result& other) {
        if (this->has_value() && other.has_value()) {
            this->m_value.swap(other.m_value);
        } else if (this->has_value() && !other.has_value()) {
            auto temp = T(move(*this).value());
            this->m_value.destroy();
            construct_at(&this->m_error, move(other).error());
            other.m_error.destroy();
            construct_at(&other.m_value, forward<T>(temp));
            ::swap(this->m_has_value, other.m_has_value);
        } else if (!this->has_value() && other.has_value()) {
            other.swap(*this);
        } else {
            this->m_error.swap(other.m_error);
        }
    }

    constexpr ErrType try_did_fail() && {
        assert(is_error());
        return ErrType(move(*this).error());
    }

    constexpr Detail::Wrapper<T> try_did_succeed() && {
        assert(has_value());
        return { move(*this).value() };
    }

private:
    bool m_has_value { true };
    union {
        Detail::UniformStorage<T> m_value;
        Detail::UniformStorage<E> m_error;
    };
};

template<typename E>
class Result<void, E> {
public:
    using ValueType = void;
    using ErrorType = E;
    using ErrType = Err<E>;

    constexpr Result() = default;
    constexpr Result(const Result&) = delete;
    constexpr Result(Result&&) = default;

    template<typename G>
    requires(ConstructibleFrom<E, G>) constexpr Result(Err<G>&& other) : m_error(move(other).error()) {}

    template<typename G>
    requires(ConstructibleFrom<E, G>) constexpr Result(Result<void, G>&& other) : m_error(move(other).m_error) {}

    constexpr ~Result() = default;

    template<typename G>
    requires(AssignableFrom<E, G>) constexpr Result& operator=(Err<G>&& other) { m_error = move(other).error(); }

    template<typename G>
    requires(AssignableFrom<E, G>) constexpr Result& operator=(Result<void, G>&& other) { m_error = move(other).m_error; }

    constexpr explicit operator bool() const { return has_value(); }
    constexpr bool has_value() const { return !m_error.has_value(); }
    constexpr bool is_error() const { return !has_value(); }

    constexpr void operator*() const { assert(has_value()); }
    constexpr void value() { assert(has_value()); }

    constexpr const E& error() const& { return m_error.value(); }
    constexpr E& error() & { return m_error.value(); }
    constexpr const E&& error() const&& { return move(m_error).value(); }
    constexpr E&& error() && { return move(m_error).value(); }

    template<EqualComparable<E> G>
    friend constexpr bool operator==(const Result& a, const Result<void, G>& b) {
        return a.m_error == b.m_error;
    }

    template<EqualComparable<E> G>
    friend constexpr bool operator==(const Result& a, const Err<G>& b) {
        return a.is_error() && a.error() == b.error();
    }

    template<typename F>
    constexpr Result<typename InvokeResult<F>::type, E> and_then(F&& f) && {
        if (has_value()) {
            return forward<F>(f)();
        } else {
            return Err<E>(move(*this).error());
        }
    }

    template<typename F>
    constexpr Result<typename InvokeResult<F>::type, E> transform(F&& f) && {
        if (has_value()) {
            return forward<F>(f)();
        } else {
            return Err<E>(move(*this).error());
        }
    }

    template<typename F>
    constexpr Result<void, typename InvokeResult<F, E&&>::type> transform_error(F&& f) && {
        if (has_value()) {
            return {};
        } else {
            return Err<typename InvokeResult<F, E&&>::type>(forward<F>(f)(move(*this).error()));
        }
    }

    constexpr void swap(Result& other) { m_error.swap(other.m_error); }

    constexpr ErrType try_did_fail() && {
        assert(is_error());
        return ErrType(move(*this).error());
    }

    constexpr Detail::Wrapper<void> try_did_succeed() && {
        assert(has_value());
        return {};
    }

private:
    Option<E> m_error;
};

namespace Detail {
    template<size_t index, typename... Types>
    struct ResultSequenceHelper {
        constexpr CommonResult<void, Types...> operator()(Tuple<Types...>& values_tuple) {
            if constexpr (index == sizeof...(Types)) {
                return {};
            } else {
                auto&& value = values_tuple.template get<index>();
                if constexpr (requires {
                                  { value.is_error() } -> SameAs<bool>;
                              }) {
                    if (value.is_error()) {
                        return move(value).try_did_fail();
                    }
                }
                return ResultSequenceHelper<index + 1, Types...> {}(values_tuple);
            }
        }
    };
}

template<typename... Types>
constexpr CommonResult<Tuple<UnwrapResult<Types>...>, Types...> tuple_result_sequence(Types&&... values) {

    auto values_tuple = Tuple<Types&...>(values...);

    auto result = Detail::ResultSequenceHelper<0, Types&...> {}(values_tuple);
    if (result.is_error()) {
        return move(result).try_did_fail();
    }

    return tuple_map(values_tuple, [](auto&& value) -> UnwrapResult<decltype(value)> {
        if constexpr (IsResult<decltype(value)>) {
            return move(value).value();
        } else {
            return value;
        }
    });
}

namespace Detail {
    template<typename F, typename T>
    struct ResultAndThenReturn {
        using Type = InvokeResult<F, T>::type;
    };

    template<typename F, IsResult T>
    struct ResultAndThenReturn<F, T> {
        using ValueType = decay_t<T>::ValueType;
        using FReturn = InvokeResult<F, Like<T, ValueType>>::type;
        using Type = CommonResult<FReturn, T>;
    };
}

template<typename T, typename F>
constexpr Detail::ResultAndThenReturn<F, T>::Type result_and_then(T&& value, F&& callable) {
    if constexpr (IsResult<T>) {
        if (value) {
            return forward<F>(callable)(forward<T>(value).value());
        } else {
            return Err(forward<T>(value).error());
        }
    } else {
        return forward<F>(callable)(forward<T>(value));
    }
}

template<typename T, typename... Args>
constexpr CommonResult<decltype(create<T>(declval<UnwrapResult<Args>>()...)), Args...>
create(Args&&... args) requires(!FalliblyMemberCreateableFrom<T, Args...> && FalliblyCreateableFrom<T, Args...>) {
    auto arguments = tuple_result_sequence(forward<Args>(args)...);
    if (arguments.is_error()) {
        return move(arguments).try_did_fail();
    }

    return apply(
        [](auto&&... args) {
            return T::create(forward<decltype(args)&&>(args)...);
        },
        move(arguments).value());
}

template<typename T, typename... Args>
requires(FalliblyCreateableFrom<T, Args...>) constexpr CommonResult<void, decltype(create<T>(declval<Args>()...))> create_at(
    T* location, Args&&... args) {
    auto result = create<T>(forward<Args>(args)...);
    if (!result) {
        return result.try_did_fail();
    }
    construct_at(location, move(result).value());
    return {};
}

template<typename T, typename U>
constexpr auto assign_to(T& lvalue, U&& other) requires(FalliblyMemberAssignableFrom<T, U>) {
    return lvalue.assign(forward<U>(other));
}

template<typename T, typename U>
constexpr CommonResult<T&, decltype(create<T>(declval<U>()))> assign_to(T& lvalue, U&& other) requires(FalliblyCreateableFrom<T, U>) {
    auto result = create<T>(forward<U>(other));
    if (!result) {
        return result.try_did_fail();
    }
    return lvalue = move(result).value();
}

template<typename T>
constexpr auto clone(const T& value) requires(FalliblyCloneable<T>) {
    return value.clone();
}
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
using LIIM::Result;
using LIIM::result_and_then;
using LIIM::tuple_result_sequence;