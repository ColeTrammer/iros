#pragma once

#include <di/any/concepts/impl.h>
#include <di/concepts/constructible_from.h>
#include <di/io/interface/reader.h>

namespace di::serialization {
template<concepts::Impl<io::Reader> Reader>
class JsonDeserializer {
public:
    template<typename T>
    requires(concepts::ConstructibleFrom<Reader, T>)
    constexpr explicit JsonDeserializer(T&& reader) : m_reader(util::forward<T>(reader)) {}

private:
    Reader m_reader;
};

template<typename T>
JsonDeserializer(T&&) -> JsonDeserializer<T>;
}
