#pragma once

#include <liim/pointers.h>
#include <liim/string.h>
#include <stddef.h>
#include <stdint.h>

namespace Ext {

class MappedFile {
public:
    static UniquePtr<MappedFile> create(const String& path);

    MappedFile(uint8_t* data, size_t size) : m_data(data), m_size(size) {}
    MappedFile(const MappedFile& other) = delete;
    ~MappedFile();

    uint8_t* data() { return m_data; }
    const uint8_t* data() const { return m_data; }

    size_t size() const { return m_size; }

private:
    uint8_t* m_data { nullptr };
    size_t m_size { 0 };
};

}
