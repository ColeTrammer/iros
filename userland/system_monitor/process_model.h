#pragma once

#include <app/model.h>
#include <eventloop/timer.h>
#include <liim/vector.h>

struct proc_info;
struct proc_global_info;

class ProcessInfo {
public:
    ProcessInfo(const proc_info& info);
    void update(const proc_info& info);

    const String& name() const { return m_name; }
    size_t resident_memory() const { return m_resident_memory; }
    int priority() const { return m_priority; }
    pid_t pid() const { return m_pid; }
    const timespec& running_time() const { return m_running_time; }

private:
    String m_name;
    size_t m_resident_memory { 0 };
    int m_priority { 0 };
    pid_t m_pid { 0 };
    timespec m_running_time { 0, 0 };
};

class GlobalProcessInfo {
public:
    void update(const proc_global_info& info);

    size_t allocated_memory() const { return m_allocated_memory; }
    size_t total_memory() const { return m_total_memory; }
    uintptr_t max_memory() const { return m_max_memory; }

    uint64_t delta_idle_ticks() const { return m_delta_idle_ticks; }
    uint64_t delta_user_ticks() const { return m_delta_user_ticks; }
    uint64_t delta_kernel_ticks() const { return m_delta_kernel_ticks; }
    uint64_t delta_total_ticks() const { return delta_idle_ticks() + delta_user_ticks() + delta_kernel_ticks(); }

private:
    size_t m_allocated_memory { 0 };
    size_t m_total_memory { 0 };
    uintptr_t m_max_memory { 0 };
    uint64_t m_delta_idle_ticks { 0 };
    uint64_t m_delta_user_ticks { 0 };
    uint64_t m_delta_kernel_ticks { 0 };
    uint64_t m_idle_ticks { 0 };
    uint64_t m_user_ticks { 0 };
    uint64_t m_kernel_ticks { 0 };
};

class ProcessModel final : public App::Model {
    APP_OBJECT(ProcessModel)

public:
    ProcessModel();
    virtual void initialize() override;

    enum Column {
        Pid,
        Name,
        Memory,
        Priority,
        RunningTime,
        __Count,
    };

    virtual int row_count() const override { return m_processes.size(); }
    virtual int col_count() const override { return Column::__Count; }
    virtual App::ModelData data(const App::ModelIndex& index, int role) const override;
    virtual App::ModelData header_data(int col, int role) const override;

    void load_data();

    const GlobalProcessInfo& global_process_info() const { return m_global_process_info; }

private:
    Vector<ProcessInfo> m_processes;
    GlobalProcessInfo m_global_process_info;
    SharedPtr<App::Timer> m_timer;
};
