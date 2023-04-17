#pragma once

#include <di/container/algorithm/max.h>
#include <di/vocab/error/prelude.h>
#include <dius/config.h>

#include DIUS_PLATFORM_PATH(error.h)

namespace dius {
class PosixDomain;

using PosixCode = di::StatusCode<PosixDomain>;

class PosixDomain final : public di::StatusCodeDomain {
private:
    using Base = StatusCodeDomain;

public:
    using Value = PosixError;
    using UniqueId = Base::UniqueId;

    constexpr explicit PosixDomain(UniqueId id = 0xff261d32b71e0a8a) : Base(id) {}

    PosixDomain(PosixDomain const&) = default;
    PosixDomain(PosixDomain&&) = default;

    PosixDomain& operator=(PosixDomain const&) = default;
    PosixDomain& operator=(PosixDomain&&) = default;

    constexpr static inline PosixDomain const& get();

    virtual di::container::ErasedString name() const override { return di::container::ErasedString(u8"Posix Domain"); }

    virtual PayloadInfo payload_info() const override {
        return { sizeof(Value), sizeof(Value) + sizeof(StatusCodeDomain const*),
                 di::container::max(alignof(Value), alignof(StatusCodeDomain const*)) };
    }

protected:
    constexpr virtual bool do_failure(di::StatusCode<void> const& code) const override {
        return down_cast(code).value() != PosixError::Success;
    }

    constexpr virtual bool do_equivalent(di::StatusCode<void> const& a, di::StatusCode<void> const& b) const override {
        DI_ASSERT(a.domain() == *this);
        return b.domain() == *this && down_cast(a).value() == down_cast(b).value();
    }

    virtual di::container::ErasedString do_message(di::StatusCode<void> const& code) const override;

private:
    template<typename Domain>
    friend class StatusCode;

    constexpr PosixCode const& down_cast(di::StatusCode<void> const& code) const {
        DI_ASSERT(code.domain() == *this);
        return static_cast<PosixCode const&>(code);
    }
};

constexpr inline auto posix_domain = PosixDomain {};

constexpr inline PosixDomain const& PosixDomain::get() {
    return posix_domain;
}
}

namespace di::vocab::detail {
template<typename = void>
constexpr auto tag_invoke(di::types::Tag<di::into_status_code>, dius::PosixError error) {
    return dius::PosixCode(di::in_place, error);
}
}
