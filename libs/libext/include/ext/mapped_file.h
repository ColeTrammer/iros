#pragma once

#include <liim/byte_buffer.h>
#include <liim/forward.h>
#include <stddef.h>
#include <stdint.h>

namespace Ext {

class MappedFile {
public:
    static UniquePtr<MappedFile> try_create(const String& path, int prot, int type);
    static UniquePtr<MappedFile> try_create_with_shared_memory(const String& path, int prot);

    MappedFile(const MappedFile& other) = delete;
    ~MappedFile();

    uint8_t* data() { return m_buffer.data(); }
    const uint8_t* data() const { return m_buffer.data(); }

    bool empty() const { return m_buffer.empty(); }
    size_t size() const { return m_buffer.size(); }

    explicit MappedFile(ByteBuffer&& buffer);

private:
    ByteBuffer m_buffer;
};
}
