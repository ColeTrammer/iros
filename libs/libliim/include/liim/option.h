#pragma once

#include <assert.h>
#include <liim/forward.h>
#include <liim/utilities.h>

namespace LIIM {
class None {};

template<typename T>
union MaybeUninit {
    T value;
    None uninit;

    constexpr MaybeUninit() : uninit({}) {}
    constexpr ~MaybeUninit() {}

    constexpr void destroy() {
        value.~T();
        uninit = {};
    }
};

namespace Detail {
    template<typename T>
    class OptionStorage {
    public:
        using ValuePointer = T*;
        using ConstValuePointer = const T*;

        constexpr OptionStorage() {}
        constexpr ~OptionStorage() { reset(); }

        constexpr bool has_value() const { return m_has_value; }

        constexpr T& value() { return m_storage.value; }
        constexpr const T& value() const { return m_storage.value; }

        template<typename... Args>
        constexpr T& emplace(Args&&... args) {
            construct_at(&m_storage.value, forward<Args>(args)...);
            m_has_value = true;
            return value();
        }

        constexpr void reset() {
            if (m_has_value) {
                m_storage.destroy();
                m_has_value = false;
            }
        }

        constexpr void assign(const T& other) {
            if (m_has_value) {
                m_storage.value = other;
            } else {
                emplace(other);
                m_has_value = true;
            }
        }

        constexpr void assign(T&& other) {
            if (m_has_value) {
                m_storage.value = move(other);
            } else {
                emplace(move(other));
                m_has_value = false;
            }
        }

        constexpr bool operator==(const OptionStorage& other) const {
            return (!this->m_has_value && !other.m_has_value) || (this->m_has_value && other.m_has_value && this->value() == other.value());
        }

    private:
        MaybeUninit<T> m_storage;
        bool m_has_value { false };
    };

    template<typename T>
    requires(IsLValueReference<T>::value) class OptionStorage<T> {
    public:
        using Value = typename RemoveReference<T>::type;
        using Storage = Value*;
        using ValuePointer = Value*;
        using ConstValuePointer = const Value*;

        constexpr OptionStorage() {}
        constexpr ~OptionStorage() {}

        constexpr bool has_value() const { return !!m_storage; }

        constexpr T value() { return *m_storage; }
        constexpr const T value() const { return *m_storage; }

        constexpr T emplace(T value) {
            m_storage = &value;
            return value;
        }

        constexpr void reset() { m_storage = nullptr; }

        constexpr void assign(T other) { emplace(other); }

        constexpr bool operator==(const OptionStorage& other) const { return this->m_storage == other.m_storage; }

    private:
        Storage m_storage { nullptr };
    };
}

template<typename T>
class Option {
public:
    using Storage = Detail::OptionStorage<T>;
    using ValuePointer = Storage::ValuePointer;
    using ConstValuePointer = Storage::ConstValuePointer;

    constexpr Option() {}
    constexpr Option(None) {}
    constexpr Option(const T& value) { emplace(value); }

    constexpr Option(T&& value) requires(!IsLValueReference<T>::value) { emplace(move(value)); }

    template<typename U>
    requires(IsLValueReference<T>::value) constexpr Option(U* pointer) {
        if (pointer) {
            emplace(*pointer);
        }
    }

    constexpr Option(std::nullptr_t) requires(IsLValueReference<T>::value) {}

    constexpr Option(const Option& other) {
        if (other.has_value()) {
            emplace(other.value());
        }
    }
    constexpr Option(Option&& other) {
        if (other.has_value()) {
            emplace(move(other.value()));
            other.reset();
        }
    }

    template<typename U>
    constexpr Option(const Option<U>& other) {
        if (other.has_value()) {
            emplace(other.value());
        }
    }
    template<typename U>
    constexpr Option(Option<U>&& other) {
        if (other.has_value()) {
            emplace(move(other.value()));
            other.reset();
        }
    }

    template<typename U>
    friend class Option;

    constexpr ~Option() { reset(); }

    constexpr void reset() { m_storage.reset(); }

    constexpr Option& operator=(const Option& other) {
        if (this != &other) {
            Option<T> temp { other };
            swap(temp);
        }
        return *this;
    }

    constexpr Option& operator=(Option&& other) {
        if (this != &other) {
            Option<T> temp { move(other) };
            swap(temp);
        }
        return *this;
    }

    constexpr bool has_value() const { return m_storage.has_value(); }
    constexpr bool operator!() const { return !has_value(); }
    constexpr operator bool() const { return has_value(); }

    constexpr bool operator==(const Option& other) const { return this->m_storage == other.m_storage; }
    constexpr bool operator!=(const Option& other) const { return !(*this == other); }

    constexpr bool operator==(const T& other) const requires(!IsLValueReference<T>::value) {
        return this->has_value() && this->value() == other;
    }
    constexpr bool operator!=(const T& other) const requires(!IsLValueReference<T>::value) { return !(*this == other); }

    constexpr T& operator*() { return value(); }
    constexpr const T& operator*() const { return value(); }

    constexpr ValuePointer operator->() { return &value(); }
    constexpr ConstValuePointer operator->() const { return &value(); }

    constexpr T& value() { return m_storage.value(); }
    constexpr const T& value() const { return m_storage.value(); }

    constexpr T value_or(T default_value) const { return has_value() ? T { value() } : T { move(default_value) }; }

    template<typename... Args>
    constexpr T& emplace(Args&&... args) {
        return m_storage.emplace(forward<Args>(args)...);
    }

    constexpr void swap(Option& other) {
        if (this->has_value() && other.has_value()) {
            LIIM::swap(this->value(), other.value());
        } else if (this->has_value()) {
            other.emplace(LIIM::move(this->value()));
            this->reset();
        } else if (other.has_value()) {
            this->emplace(LIIM::move(other.value()));
            other.reset();
        }
    }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr Option<R> map(C mapper) const {
        if (!has_value()) {
            return {};
        }
        return Option<R> { mapper(value()) };
    }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr R and_then(C mapper) const {
        if (!has_value()) {
            return {};
        }
        return mapper(value());
    }

    template<typename C, typename R = InvokeResult<C>::type>
    constexpr Result<T, R> unwrap_or_else(C mapper) {
        if (has_value()) {
            return Ok<T>(forward<T&&>(value()));
        }
        return Err<R>(mapper());
    }

    constexpr None try_did_fail() const {
        assert(!has_value());
        return {};
    }

    constexpr Option<T&&> try_did_succeed() { return move(value()); }

    constexpr Option<T> try_did_succeed() requires(IsLValueReference<T>::value) { return value(); }

    constexpr T&& try_move_out() { return move(value()); }
    constexpr T try_move_out() requires(IsLValueReference<T>::value) { return value(); }

private : Storage m_storage;
};

template<typename T>
constexpr void swap(Option<T>& a, Option<T>& b) {
    a.swap(b);
}
}

using LIIM::None;
using LIIM::Option;
