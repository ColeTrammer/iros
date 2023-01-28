#pragma once

#include <liim/forward.h>

namespace Thread {
class BackgroundJob {
public:
    static void start(Function<void()> task);
};
}
