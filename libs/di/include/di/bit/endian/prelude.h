#pragma once

#include <di/bit/endian/big_endian.h>
#include <di/bit/endian/endian.h>
#include <di/bit/endian/little_endian.h>
#include <di/bit/endian/static_endian.h>

namespace di {
using bit::BigEndian;
using bit::Endian;
using bit::LittleEndian;
using bit::StaticEndian;

using bit::big_endian_to_host;
using bit::host_to_big_endian;
using bit::host_to_little_endian;
using bit::little_endian_to_host;
}
