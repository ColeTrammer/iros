#include <stdlib.h>

#include "process_model.h"

ProcessModel::ProcessModel() {
    proc_info* info;
    size_t info_len;
    if (read_procfs_info(&info, &info_len, READ_PROCFS_SCHED)) {
        perror("system_monitor: read_procfs_info");
        exit(1);
    }

    m_processes = Vector<proc_info>::wrap_dynamic_array(info, info_len);
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
        default:
            return {};
    }
}
