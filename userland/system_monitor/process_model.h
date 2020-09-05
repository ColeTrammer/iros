#pragma once

#include <app/model.h>
#include <app/timer.h>
#include <liim/vector.h>

struct proc_info;

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

class ProcessModel final : public App::Model {
    APP_OBJECT(ProcessModel)

public:
    ProcessModel();

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
    virtual App::ModelData data(const App::ModelIndex& index) const override;
    virtual App::ModelData header_data(int col) const override;

    void load_data();

private:
    Vector<ProcessInfo> m_processes;
    SharedPtr<App::Timer> m_timer;
};
