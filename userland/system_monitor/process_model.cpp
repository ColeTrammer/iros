#include <procinfo.h>
#include <stdlib.h>

#include "process_model.h"

ProcessModel::ProcessModel() {
    m_timer = App::Timer::create_interval_timer(
        nullptr,
        [this](int) {
            load_data();
        },
        1000);
    load_data();
}

ProcessInfo::ProcessInfo(const proc_info& info) {
    update(info);
}

void ProcessInfo::update(const proc_info& info) {
    m_name = info.name;
    m_resident_memory = info.resident_memory;
    m_priority = info.priority;
    m_pid = info.pid;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    m_running_time = { .tv_sec = now.tv_sec - info.start_time.tv_sec, .tv_nsec = now.tv_nsec - info.start_time.tv_nsec };
}

void ProcessModel::load_data() {
    proc_info* info;
    size_t num_processes;
    if (read_procfs_info(&info, &num_processes, READ_PROCFS_SCHED)) {
        perror("system_monitor: read_procfs_info");
        exit(1);
    }

    // read_procfs_info() returns the process information sorted by pid. In order to sample things like the process CPU usage,
    // it is necessary to match up the new process information with the old. To accomplish this, the new information must be
    // merged with the new.
    size_t new_info_index = 0;
    size_t old_info_index = 0;
    while (new_info_index < num_processes && old_info_index < static_cast<size_t>(m_processes.size())) {
        auto& new_info = info[new_info_index];
        auto& old_info = m_processes[old_info_index];
        if (new_info.pid == old_info.pid()) {
            old_info.update(new_info);
            new_info_index++;
            old_info_index++;
            continue;
        }

        if (new_info.pid < old_info.pid()) {
            m_processes.insert(new_info, old_info_index);
            new_info_index++;
            old_info_index++;
            continue;
        }

        m_processes.remove(old_info_index);
    }

    while (old_info_index < static_cast<size_t>(m_processes.size())) {
        m_processes.remove(old_info_index);
    }

    while (new_info_index < num_processes) {
        m_processes.add(info[new_info_index++]);
    }

    free_procfs_info(info);

    did_update();
}

App::ModelData ProcessModel::data(const App::ModelIndex& index) const {
    int row = index.row();
    if (row < 0 || row >= m_processes.size()) {
        return {};
    }

    auto& process = m_processes[row];
    switch (index.col()) {
        case Column::Pid:
            return String::format("%d", process.pid());
        case Column::Name:
            return process.name();
        case Column::Memory:
            return String::format("%lu", process.resident_memory());
        case Column::Priority:
            return String::format("%d", process.priority());
        case Column::RunningTime: {
            double seconds = process.running_time().tv_sec + process.running_time().tv_nsec / 1000000000.0;
            long int_seconds = (int) seconds;
            seconds -= int_seconds;
            seconds *= 100;

            long minutes = int_seconds / 60;
            int_seconds %= 60;

            return String::format("%ld:%02ld.%02d", minutes, int_seconds, (int) seconds);
        }
        default:
            return {};
    }
}

App::ModelData ProcessModel::header_data(int col) const {
    switch (col) {
        case Column::Pid:
            return "PID";
        case Column::Name:
            return "Name";
        case Column::Memory:
            return "Memory";
        case Column::Priority:
            return "Priority";
        case Column::RunningTime:
            return "Running Time";
        default:
            return {};
    }
}
