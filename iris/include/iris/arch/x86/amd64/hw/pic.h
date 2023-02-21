#pragma once

namespace iris::x86::amd64 {
void init_pic();

void send_eoi(u8 irq_line);
}
