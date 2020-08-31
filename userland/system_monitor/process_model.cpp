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

void ProcessModel::load_data() {
    proc_info* info;
    size_t info_len;
    if (read_procfs_info(&info, &info_len, READ_PROCFS_SCHED)) {
        perror("system_monitor: read_procfs_info");
        exit(1);
    }

    m_processes = Vector<proc_info>::wrap_dynamic_array(info, info_len);
    did_update();
}

App::ModelData ProcessModel::data(const App::ModelIndex& index) const {
    int row = index.row();
    if (row < 0 || row >= m_processes.size()) {
        return {};
    }

    auto& process = m_processes[row];
    switch (index.col()) {
        case Column::Name:
            return process.name;
        case Column::Memory:
            return String::format("%lu", process.resident_memory);
        case Column::Priority:
            return String::format("%d", process.priority);
        case Column::RunningTime: {
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            struct timespec delta = { .tv_sec = now.tv_sec - process.start_time.tv_sec,
                                      .tv_nsec = now.tv_nsec - process.start_time.tv_nsec };
            double seconds = delta.tv_sec + delta.tv_nsec / 1000000000.0;
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
