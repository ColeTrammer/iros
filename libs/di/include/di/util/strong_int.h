#pragma once

#include <di/format/prelude.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/language.h>

namespace di::util {
namespace detail {
    struct NoopMixin {};

    template<typename T>
    struct MixinHelper : meta::TypeConstant<NoopMixin> {};

    template<typename T>
    requires(requires { typename T::Mixin; })
    struct MixinHelper<T> : meta::TypeConstant<typename T::Mixin> {};

    template<typename T>
    using Mixin = meta::Type<MixinHelper<T>>;

    template<typename T>
    concept Tag = requires { typename T::Type; };

    template<typename T>
    struct SSizeTypeHelper : meta::TypeConstant<meta::MakeSigned<meta::Type<T>>> {};

    template<typename T>
    requires(requires { typename T::SSizeType; })
    struct SSizeTypeHelper<T> : meta::TypeConstant<typename T::SSizeType> {};

    template<Tag T>
    using SSizeType = meta::Type<SSizeTypeHelper<T>>;

    template<typename T>
    constexpr inline bool format_as_pointer = false;

    template<typename T>
    requires(T::format_as_pointer)
    constexpr inline bool format_as_pointer<T> = true;
}

template<detail::Tag Tag>
class StrongInt : public detail::Mixin<Tag> {
public:
    using Type = Tag::Type;
    using SSizeType = detail::SSizeType<Tag>;

    StrongInt() = default;

    constexpr explicit StrongInt(Type value) : m_value(value) {}

    constexpr Type raw_value() const { return m_value; }

    constexpr StrongInt& operator+=(Type x) {
        m_value += x;
        return *this;
    }
    constexpr StrongInt& operator+=(SSizeType x)
    requires(!concepts::SameAs<SSizeType, Type>)
    {
        m_value += x;
        return *this;
    }

    constexpr StrongInt& operator-=(Type x) {
        m_value -= x;
        return *this;
    }
    constexpr StrongInt& operator-=(SSizeType x)
    requires(!concepts::SameAs<SSizeType, Type>)
    {
        m_value -= x;
        return *this;
    }

    constexpr StrongInt& operator++() {
        ++m_value;
        return *this;
    }
    constexpr StrongInt operator++(int) {
        auto copy = *this;
        ++m_value;
        return copy;
    }

    constexpr StrongInt& operator--() {
        --m_value;
        return *this;
    }
    constexpr StrongInt operator--(int) {
        auto copy = *this;
        --m_value;
        return copy;
    }

private:
    constexpr friend StrongInt operator+(StrongInt a, Type b) { return StrongInt(a.raw_value() + b); }
    constexpr friend StrongInt operator+(StrongInt a, SSizeType b)
    requires(!concepts::SameAs<SSizeType, Type>)
    {
        return StrongInt(a.raw_value() + b);
    }
    constexpr friend StrongInt operator+(Type a, StrongInt b) { return StrongInt(a + b.raw_value()); }
    constexpr friend StrongInt operator+(SSizeType a, StrongInt b)
    requires(!concepts::SameAs<SSizeType, Type>)
    {
        return StrongInt(a + b.raw_value());
    }

    constexpr friend StrongInt operator-(StrongInt a, Type b) { return StrongInt(a.raw_value() - b); }
    constexpr friend StrongInt operator-(StrongInt a, SSizeType b)
    requires(!concepts::SameAs<SSizeType, Type>)
    {
        return StrongInt(a.raw_value() - b);
    }
    constexpr friend StrongInt operator-(Type a, StrongInt b) { return StrongInt(a - b.raw_value()); }
    constexpr friend StrongInt operator-(SSizeType a, StrongInt b)
    requires(!concepts::SameAs<SSizeType, Type>)
    {
        return StrongInt(a - b.raw_value());
    }

    constexpr friend SSizeType operator-(StrongInt a, StrongInt b) { return a.raw_value() - b.raw_value(); }

    constexpr friend bool operator==(StrongInt a, StrongInt b) { return a.raw_value() == b.raw_value(); }
    constexpr friend di::strong_ordering operator<=>(StrongInt a, StrongInt b) {
        return a.raw_value() <=> b.raw_value();
    }

    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<format::formatter_in_place>, InPlaceType<StrongInt>,
                                     FormatParseContext<Enc>& parse_context, bool debug) {
        using Base = meta::Conditional<detail::format_as_pointer<Tag>, void*, Type>;
        return format::formatter<Base, Enc>(parse_context, debug) % [](concepts::CopyConstructible auto formatter) {
            return [=](concepts::FormatContext auto& context, StrongInt self) {
                if constexpr (concepts::Pointer<Base>) {
                    auto* pointer = reinterpret_cast<void*>(self.raw_value());
                    return formatter(context, pointer);
                } else {
                    return formatter(context, self.raw_value());
                }
            };
        };
    }

    Type m_value { 0 };
};
}
