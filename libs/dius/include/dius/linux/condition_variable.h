#pragma once

#ifndef DIUS_USE_RUNTIME
#include "dius/posix/condition_variable.h"
#else
#include "di/sync/dumb_condition_variable.h"

namespace dius {
using ConditionVariable = di::sync::DumbConditionVariable;
}
#endif
