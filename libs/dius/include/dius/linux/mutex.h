#pragma once

#ifndef DIUS_USE_RUNTIME
#include "dius/posix/mutex.h"
#else
namespace di::sync {
class DumbSpinlock;
}
namespace dius {
using Mutex = di::sync::DumbSpinlock;
}
#endif
