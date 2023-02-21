#pragma once

#include <di/container/algorithm/max.h>
#include <di/container/string/string_view.h>
#include <di/types/prelude.h>
#include <di/vocab/error/status_code.h>
#include <di/vocab/error/status_code_domain.h>

namespace di::vocab {
inline namespace generic_error {
    enum class BasicError : long {
        Success = 0,
        FailedAllocation,
        OutOfRange,
        Invalid,
        Cancelled,
    };

    template<typename = void>
    constexpr auto tag_invoke(types::Tag<into_status_code>, BasicError error) {
        return GenericCode(in_place, error);
    }
}

class GenericDomain final : public StatusCodeDomain {
private:
    using Base = StatusCodeDomain;

public:
    using Value = BasicError;
    using UniqueId = Base::UniqueId;

    constexpr explicit GenericDomain(UniqueId id = 0x25657faae58bbe11) : Base(id) {}

    GenericDomain(GenericDomain const&) = default;
    GenericDomain(GenericDomain&&) = default;

    GenericDomain& operator=(GenericDomain const&) = default;
    GenericDomain& operator=(GenericDomain&&) = default;

    constexpr static inline GenericDomain const& get();

    virtual container::ErasedString name() const override { return u8"Generic Domain"_sv; }

    virtual PayloadInfo payload_info() const override {
        return { sizeof(Value), sizeof(Value) + sizeof(StatusCodeDomain const*),
                 container::max(alignof(Value), alignof(StatusCodeDomain const*)) };
    }

protected:
    constexpr virtual bool do_failure(StatusCode<void> const& code) const override {
        return down_cast(code).value() != BasicError::Success;
    }

    constexpr virtual bool do_equivalent(StatusCode<void> const& a, StatusCode<void> const& b) const override {
        DI_ASSERT_EQ(a.domain(), *this);
        return b.domain() == *this && down_cast(a).value() == down_cast(b).value();
    }

    constexpr virtual container::ErasedString do_message(StatusCode<void> const& code) const override {
        using namespace di::string_literals;

        auto value = down_cast(code).value();
        switch (value) {
            case BasicError::Success:
                return "Success"_sv;
            case BasicError::FailedAllocation:
                return "Allocation Failed"_sv;
            case BasicError::OutOfRange:
                return "Out of Range"_sv;
            case BasicError::Invalid:
                return "Invalid"_sv;
            default:
                return "Unknown"_sv;
        }
    }

private:
    template<typename Domain>
    friend class StatusCode;

    constexpr GenericCode const& down_cast(StatusCode<void> const& code) const {
        DI_ASSERT_EQ(code.domain(), *this);
        return static_cast<GenericCode const&>(code);
    }
};

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

constexpr inline GenericDomain const& GenericDomain::get() {
    return generic_domain;
}
}
