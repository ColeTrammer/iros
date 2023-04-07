#pragma once

namespace iris {
class Processor;

inline Processor& current_processor() {
    auto* result = static_cast<Processor*>(nullptr);
    asm volatile("mov %%gs:0, %0" : "=r"(result));
    return *result;
}
}
