#pragma once

#include <liim/byte_buffer.h>
#include <liim/forward.h>
#include <stddef.h>
#include <stdint.h>

namespace Ext {
UniquePtr<ByteBuffer> try_map_file(const String& path, int prot, int type);
UniquePtr<ByteBuffer> try_map_shared_memory(const String& path, int prot);
}
