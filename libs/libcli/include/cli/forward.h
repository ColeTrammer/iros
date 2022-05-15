#pragma once

#include <stddef.h>

namespace Cli {
class Argument;

class Flag;

template<typename T, size_t flag_count, size_t argument_count>
class Parser;
}
