#pragma once

#include <di/container/string/erased_string.h>
#include <di/types/prelude.h>
#include <di/vocab/error/status_code_forward_declaration.h>

namespace di::vocab {
class StatusCodeDomain {
public:
    using UniqueId = u64;

    constexpr auto id() const { return m_id; }

    constexpr virtual auto name() const -> container::ErasedString = 0;

    struct PayloadInfo {
        size_t payload_size;
        size_t total_size;
        size_t total_alignment;

        PayloadInfo() = default;
        constexpr PayloadInfo(size_t payload_size_, size_t total_size_, size_t total_aignment_)
            : payload_size(payload_size_), total_size(total_size_), total_alignment(total_aignment_) {}
    };

    constexpr virtual auto payload_info() const -> PayloadInfo = 0;

protected:
    constexpr explicit StatusCodeDomain(UniqueId id) : m_id(id) {}

    StatusCodeDomain(StatusCodeDomain const&) = default;
    StatusCodeDomain(StatusCodeDomain&&) = default;

    auto operator=(StatusCodeDomain const&) -> StatusCodeDomain& = default;
    auto operator=(StatusCodeDomain&&) -> StatusCodeDomain& = default;

    ~StatusCodeDomain() = default;

    constexpr virtual auto do_failure(StatusCode<void> const&) const -> bool = 0;
    constexpr virtual auto do_equivalent(StatusCode<void> const&, StatusCode<void> const&) const -> bool = 0;
    constexpr virtual auto do_message(StatusCode<void> const&) const -> container::ErasedString = 0;
    constexpr virtual auto do_convert_to_generic(StatusCode<void> const&) const -> GenericCode = 0;
    constexpr virtual void do_erased_destroy(StatusCode<void>&, size_t) const {
        // This is a no-op for trivially copyable types, but must be
        // customized for trivially relocable types (UniquePtr) to
        // prevent memory leaks.
    }

private:
    template<typename Domain>
    friend class StatusCode;

    constexpr friend auto operator==(StatusCodeDomain const& a, StatusCodeDomain const& b) -> bool {
        return a.id() == b.id();
    }
    constexpr friend auto operator<=>(StatusCodeDomain const& a, StatusCodeDomain const& b) -> strong_ordering {
        return a.id() <=> b.id();
    }

    UniqueId m_id;
};
}
