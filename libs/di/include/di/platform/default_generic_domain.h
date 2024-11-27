#pragma once

#include <di/container/algorithm/max.h>
#include <di/types/prelude.h>
#include <di/vocab/error/status_code.h>
#include <di/vocab/error/status_code_domain.h>

namespace di::platform {
inline namespace generic_error {
    enum class BasicError : long {
        Success = 0,
        NotEnoughMemory,
        ResultOutOfRange,
        InvalidArgument,
        ValueTooLarge,
        OperationCanceled,
        Cancelled,
    };
}

class GenericDomain final : public vocab::StatusCodeDomain {
private:
    using Base = StatusCodeDomain;

public:
    using Value = BasicError;
    using UniqueId = Base::UniqueId;

    constexpr explicit GenericDomain(UniqueId id = 0x25657faae58bbe11) : Base(id) {}

    GenericDomain(GenericDomain const&) = default;
    GenericDomain(GenericDomain&&) = default;

    auto operator=(GenericDomain const&) -> GenericDomain& = default;
    auto operator=(GenericDomain&&) -> GenericDomain& = default;

    constexpr static inline auto get() -> GenericDomain const&;

    virtual auto name() const -> container::ErasedString override {
        return container::ErasedString(u8"Generic Domain");
    }

    virtual auto payload_info() const -> PayloadInfo override {
        return { sizeof(Value), sizeof(Value) + sizeof(StatusCodeDomain const*),
                 container::max(alignof(Value), alignof(StatusCodeDomain const*)) };
    }

protected:
    constexpr virtual auto do_failure(vocab::StatusCode<void> const& code) const -> bool override {
        return down_cast(code).value() != BasicError::Success;
    }

    constexpr virtual auto do_equivalent(vocab::StatusCode<void> const& a, vocab::StatusCode<void> const& b) const
        -> bool override {
        DI_ASSERT(a.domain() == *this);
        return b.domain() == *this && down_cast(a).value() == down_cast(b).value();
    }

    constexpr virtual auto do_convert_to_generic(vocab::StatusCode<void> const& a) const
        -> vocab::GenericCode override {
        DI_ASSERT(a.domain() == *this);
        return vocab::GenericCode(di::in_place, down_cast(a).value());
    }

    constexpr virtual auto do_message(vocab::StatusCode<void> const& code) const -> container::ErasedString override {
        auto value = down_cast(code).value();
        switch (value) {
            case BasicError::Success:
                return container::ErasedString(u8"Success");
            case BasicError::NotEnoughMemory:
                return container::ErasedString(u8"Not enough memory");
            case BasicError::ResultOutOfRange:
                return container::ErasedString(u8"Result out of range");
            case BasicError::InvalidArgument:
                return container::ErasedString(u8"Invalid argument");
            default:
                return container::ErasedString(u8"Unknown");
        }
    }

private:
    template<typename Domain>
    friend class StatusCode;

    constexpr auto down_cast(vocab::StatusCode<void> const& code) const -> vocab::GenericCode const& {
        DI_ASSERT(code.domain() == *this);
        return static_cast<vocab::GenericCode const&>(code);
    }
};

inline namespace generic_error {
    constexpr auto tag_invoke(types::Tag<vocab::into_status_code>, BasicError error) {
        return vocab::GenericCode(in_place, error);
    }
}

#ifdef DI_SANITIZER
// When compiling with UBSAN, using the address of a constexpr inline variable fails.
// This includes checking for nullptr. To work around this, do not declare the variable
// as inline when compiling with a sanitizer.
// See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71962.
// As a side note, this means there will be multiple copies of the generic_domain object
// in a user's program. This is perfectly fine, since we make sure to compare domains by
// their unique id and not their address, which is necessary even for inline variables when
// in the presence of dynamic linking.
constexpr auto generic_domain = GenericDomain {};
#else
constexpr inline auto generic_domain = GenericDomain {};
#endif

constexpr inline auto GenericDomain::get() -> GenericDomain const& {
    return generic_domain;
}
}

namespace di::vocab {
constexpr auto StatusCode<void>::generic_code() const -> GenericCode {
    if (!this->empty()) {
        return this->domain().do_convert_to_generic(*this);
    }
    return GenericCode(di::in_place, platform::BasicError::InvalidArgument);
}
}
