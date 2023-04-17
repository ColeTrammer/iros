#pragma once

#include <di/concepts/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <di/meta/prelude.h>
#include <iris/core/error.h>
#include <iris/core/userspace_access.h>

namespace iris {
template<di::concepts::ImplicitLifetime T>
class UserspaceBuffer {
private:
    using Storage = di::meta::RemoveCV<T>;

    constexpr static bool is_const = di::concepts::Const<T>;
    constexpr static bool is_byte = di::concepts::SameAs<Storage, byte>;

    explicit UserspaceBuffer(T* pointer, usize length) : m_buffer(pointer, length) {}

public:
    static Expected<UserspaceBuffer<T>> create(T* pointer, usize length) {
        if (!validate_user_region(mm::VirtualAddress(di::to_uintptr(pointer)), length, sizeof(T))) {
            return di::Unexpected(Error::BadAddress);
        }
        return UserspaceBuffer<T> { pointer, length };
    }

    Expected<usize> write(di::Span<T const> data) const
    requires(!is_const)
    {
        auto to_write = di::min(data.size_bytes(), m_buffer.size_bytes());
        TRY(copy_to_user(*di::as_bytes(data).first(to_write), reinterpret_cast<byte*>(m_buffer.data())));
        return to_write;
    }

    Expected<usize> copy_to(di::Span<Storage> buffer) const {
        auto to_read = di::min(buffer.size_bytes(), m_buffer.size_bytes());
        TRY(copy_from_user(*di::as_bytes(m_buffer).first(to_read), reinterpret_cast<byte*>(buffer.data())));
        return to_read;
    }

    Expected<di::TransparentString> copy_to_string() const
    requires(is_byte)
    {
        auto string = di::TransparentString {};
        TRY(string.reserve_from_nothing(size()));
        string.assume_size(size());
        TRY(copy_to({ reinterpret_cast<byte*>(string.span().data()), string.size() }));
        return string;
    }

    Expected<di::Path> copy_to_path() const
    requires(is_byte)
    {
        if (size() > 4096) {
            return di::Unexpected(Error::FilenameTooLong);
        }
        auto string = TRY(copy_to_string());
        return di::Path(di::move(string));
    }

    template<usize chunk_size>
    Expected<void> copy_in_chunks(di::FunctionRef<Expected<void>(di::Span<Storage>)> process_chunk) const {
        auto buffer = di::Array<byte, sizeof(Storage) * chunk_size> {};
        for (auto offset : di::range(size_bytes()) | di::stride(sizeof(Storage) * chunk_size)) {
            auto to_read = di::min(sizeof(Storage) * chunk_size, size_bytes() - offset);
            auto userspace_buffer = reinterpret_cast<byte const*>(m_buffer.data()) + offset;
            TRY(copy_from_user({ userspace_buffer, to_read }, buffer.data()));
            TRY(process_chunk({ reinterpret_cast<Storage*>(buffer.data()), to_read / sizeof(Storage) }));
        }
        return {};
    }

    usize size() const { return m_buffer.size(); }
    usize size_bytes() const { return m_buffer.size_bytes(); }
    [[nodiscard]] bool empty() const { return m_buffer.empty(); }

private:
    di::Span<T> m_buffer;
};

template<typename T>
UserspaceBuffer<T> tag_invoke(di::Tag<di::util::deduce_create>, di::InPlaceTemplate<UserspaceBuffer>, T*, usize);
}
