#pragma once

namespace iris::arch {
// This functions setup the C++ runtime enviornment. In practice,
// this should involve calling the "global" constructors, which
// typically requires help from the architecture specific linker
// script.
void cxx_init();
}
