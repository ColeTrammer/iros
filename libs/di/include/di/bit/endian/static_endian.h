#pragma once

#include "di/bit/endian/big_endian.h"
#include "di/bit/endian/endian.h"
#include "di/bit/endian/little_endian.h"

namespace di::bit {
template<concepts::IntegralOrEnum T, Endian endian>
using StaticEndian = meta::Conditional<endian == Endian::Little, LittleEndian<T>, BigEndian<T>>;
}

namespace di {
using bit::StaticEndian;
}
