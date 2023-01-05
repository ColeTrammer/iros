#pragma once

#include <di/sync/atomic.h>
#include <di/sync/concepts/stoppable_token.h>
#include <di/sync/concepts/stoppable_token_for.h>
#include <di/sync/concepts/unstoppable_token.h>
#include <di/sync/dumb_spinlock.h>
#include <di/sync/memory_order.h>
#include <di/sync/never_stop_token.h>
#include <di/sync/scoped_lock.h>
#include <di/sync/synchronized.h>

namespace di {
using sync::Atomic;
using sync::DumbSpinlock;
using sync::MemoryOrder;
using sync::NeverStopToken;
using sync::ScopedLock;
using sync::Synchronized;
}