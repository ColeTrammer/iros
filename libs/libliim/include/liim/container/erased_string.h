#pragma once

#include <liim/erased.h>

namespace LIIM::Container {
namespace Detail {
    constexpr size_t erased_storage_size = 3 * sizeof(void*);
    constexpr size_t erased_storage_alignment = alignof(void*);

    template<typename T>
    concept StringErasable = Erasable<T, erased_storage_size, erased_storage_alignment>;

    template<typename T = void>
    using ErasedStringStorage = ErasedStorage<T, erased_storage_size, erased_storage_alignment>;

    template<StringErasable T>
    ErasedStringStorage<T> erased_string_storage_cast(ErasedStringStorage<> value) {
        return erased_storage_cast<T>(value);
    }
}

class ErasedString {
private:
    using DataAccessor = const char* (*) (Detail::ErasedStringStorage<>);
    using SizeAccessor = size_t (*)(Detail::ErasedStringStorage<>);
    using Destroyer = void (*)(Detail::ErasedStringStorage<>);

public:
    ErasedString(ErasedString&& other)
        : m_storage(move(other.m_storage))
        , m_data_accessor(exchange(other.m_data_accessor, nullptr))
        , m_size_accessor(exchange(other.m_size_accessor, nullptr))
        , m_destroyer(exchange(other.m_destroyer, nullptr)) {}

    ErasedString(Detail::ErasedStringStorage<> storage, DataAccessor data_accessor, SizeAccessor size_accessor, Destroyer destroyer)
        : m_storage(storage), m_data_accessor(data_accessor), m_size_accessor(size_accessor), m_destroyer(destroyer) {}

    ~ErasedString() {
        if (m_destroyer) {
            m_destroyer(m_storage);
        }
    }

    const char* data() const {
        assert(m_data_accessor);
        return m_data_accessor(m_storage);
    }

    size_t size() const {
        assert(m_size_accessor);
        return m_size_accessor(m_storage);
    }

    bool empty() const { return size() == 0; }

    const char* begin() const { return data(); }
    const char* end() const { return data() + size(); }

private:
    Detail::ErasedStringStorage<> m_storage;
    DataAccessor m_data_accessor;
    SizeAccessor m_size_accessor;
    Destroyer m_destroyer;
};
}

using LIIM::Container::ErasedString;
