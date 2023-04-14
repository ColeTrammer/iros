#pragma once

#include <di/prelude.h>

#include <iris/core/object_pool.h>
#include <iris/core/preemption.h>
#include <iris/core/scheduler.h>

#include IRIS_ARCH_INCLUDE(core/processor.h)

namespace iris {
struct IpiMessage {
    di::Atomic<u32> times_processed { 0 };
    mm::VirtualAddress tlb_flush_base {};
    usize tlb_flush_size { 0 };
    Task* task_to_schedule { nullptr };
};

class Processor : public di::SelfPointer<Processor> {
public:
    Processor() = default;

    explicit Processor(u16 id) : m_id(id) {}

    u16 id() const { return m_id; }
    Scheduler& scheduler() { return m_scheduler; }

    void mark_as_initialized() { m_is_initialized.store(true, di::MemoryOrder::Release); }
    bool is_initialized() const { return m_is_initialized.load(di::MemoryOrder::Acquire); }

    void mark_as_booted() { m_is_booted.store(true, di::MemoryOrder::Release); }
    bool is_booted() const { return m_is_booted.load(di::MemoryOrder::Acquire); }

    void mark_as_online() { m_is_online.store(true, di::MemoryOrder::Relaxed); }
    bool is_online() const { return m_is_online.load(di::MemoryOrder::Relaxed); }

    arch::ArchProcessor& arch_processor() { return m_arch_processor; }

    void send_ipi(u32 target_processor_id, di::FunctionRef<void(IpiMessage&)> factory);
    void broadcast_ipi(di::FunctionRef<void(IpiMessage&)> factory);

    void handle_pending_ipi_messages();

    void flush_tlb_local(mm::VirtualAddress base, usize byte_length);
    void flush_tlb_local();

private:
    Scheduler m_scheduler;
    di::Atomic<bool> m_is_initialized { false };
    di::Atomic<bool> m_is_booted { false };
    di::Atomic<bool> m_is_online { false };
    di::Synchronized<di::Queue<IpiMessage*, di::StaticRing<IpiMessage*, di::meta::SizeConstant<32>>>>
        m_ipi_message_queue {};
    u16 m_id {};
    arch::ArchProcessor m_arch_processor;
};

/// @brief Setups access to the current processor.
///
/// This function is called whenever a context switch from lower privilege levels to higher privilege levels is
/// performed. On x86_64, this loads the GS segment register with the address of the current processor using `swapgs`.
void setup_current_processor_access();

/// @brief Sets the current processor address.
///
/// @warning This function can only be called once, during each processor's initialization.
void set_current_processor(Processor& processor);

inline auto current_processor() {
    auto guard = PreemptionDisabler {};
    // SAFETY: This is safe since preemption is disabled.
    auto& processor = current_processor_unsafe();
    return di::GuardedReference<Processor, PreemptionDisabler> { processor, di::move(guard) };
}

inline auto current_scheduler() {
    auto guard = PreemptionDisabler {};
    // SAFETY: This is safe since preemption is disabled.
    auto& processor = current_processor_unsafe();
    return di::GuardedReference<Scheduler, PreemptionDisabler> { processor.scheduler(), di::move(guard) };
}
}
