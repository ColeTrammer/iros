#pragma once

#include <di/prelude.h>
#include <iris/core/error.h>
#include <iris/core/userspace_access.h>

namespace iris {
namespace detail {
    template<bool is_const>
    class UserspaceBuffer {
    private:
        using Storage = di::meta::MaybeConst<is_const, byte>;

    public:
        explicit UserspaceBuffer(Storage* pointer, usize length) : m_buffer(pointer, length) {}

        Expected<usize> write(di::Span<byte const> data) const
        requires(!is_const)
        {
            auto to_write = di::min(data.size(), m_buffer.size());
            TRY(copy_to_user(*data.first(to_write), m_buffer.data()));
            return to_write;
        }

        Expected<usize> copy_to(di::Span<byte> buffer) const {
            auto to_read = di::min(buffer.size(), m_buffer.size());
            TRY(copy_from_user(m_buffer, buffer.data()));
            return to_read;
        }

        Expected<di::TransparentString> copy_to_string() const {
            auto string = di::TransparentString {};
            TRY(string.reserve_from_nothing(size()));
            TRY(copy_to({ reinterpret_cast<byte*>(string.span().data()), string.size() }));
            string.assume_size(size());
            return string;
        }

        Expected<di::Path> copy_to_path() const {
            if (size() > 4096) {
                return di::Unexpected(Error::FilenameTooLong);
            }
            auto string = TRY(copy_to_string());
            return di::Path(di::move(string));
        }

        template<usize chunk_size>
        Expected<void> copy_in_chunks(di::FunctionRef<Expected<void>(di::Span<byte>)> process_chunk) const {
            auto buffer = di::Array<byte, chunk_size> {};
            for (auto offset : di::range(size()) | di::stride(chunk_size)) {
                auto to_read = di::min(chunk_size, size() - offset);
                TRY((copy_from_user({ m_buffer.data() + offset, to_read }, buffer.data())));
                TRY(process_chunk(*buffer.first(to_read)));
            }
            return {};
        }

        usize size() const { return m_buffer.size(); }
        [[nodiscard]] bool empty() const { return m_buffer.empty(); }

    private:
        di::Span<Storage> m_buffer;
    };
}

using WritableUserspaceBuffer = detail::UserspaceBuffer<false>;
using ReadonlyUserspaceBuffer = detail::UserspaceBuffer<true>;
}
