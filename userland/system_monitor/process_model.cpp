#include <procinfo.h>
#include <stdlib.h>

#include "process_model.h"

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

void GlobalProcessInfo::update(const proc_global_info& info) {
    m_allocated_memory = info.allocated_memory;
    m_total_memory = info.total_memory;
    m_max_memory = info.max_memory;
    m_delta_idle_ticks = info.idle_ticks - m_idle_ticks;
    m_delta_user_ticks = info.user_ticks - m_user_ticks;
    m_delta_kernel_ticks = info.kernel_ticks - m_kernel_ticks;
    m_idle_ticks = info.idle_ticks;
    m_user_ticks = info.user_ticks;
    m_kernel_ticks = info.kernel_ticks;
}

ProcessModel::ProcessModel() {}

void ProcessModel::initialize() {
    m_timer = App::Timer::create_interval_timer(nullptr, 1000);
    m_timer->on<App::TimerEvent>(*this, [this](auto&) {
        load_data();
    });
    load_data();
}

void ProcessModel::load_data() {
    proc_global_info global_info;
    if (read_procfs_global_info(&global_info, READ_PROCFS_GLOBAL_MEMINFO | READ_PROCFS_GLOBAL_SCHED)) {
        perror("system_monitor: read_procfs_global_info");
        exit(1);
    }
    m_global_process_info.update(global_info);

    proc_info* info;
    size_t num_processes;
    if (read_procfs_info(&info, &num_processes, READ_PROCFS_SCHED)) {
        perror("system_monitor: read_procfs_info");
        exit(1);
    }

    // read_procfs_info() returns the process information sorted by pid. In order to sample things like the process CPU usage,
    // it is necessary to match up the new process information with the old. To accomplish this, the new information must be
    // merged with the new.
    auto* root_item = model_item_root();
    size_t new_info_index = 0;
    size_t old_info_index = 0;
    while (new_info_index < num_processes && old_info_index < static_cast<size_t>(root_item->item_count())) {
        auto& new_info = info[new_info_index];
        auto& old_info = root_item->typed_item<ProcessInfo>(old_info_index);
        if (new_info.pid == old_info.pid()) {
            old_info.update(new_info);
            new_info_index++;
            old_info_index++;
            continue;
        }

        if (new_info.pid < old_info.pid()) {
            insert_child<ProcessInfo>(*root_item, old_info_index, new_info);
            new_info_index++;
            old_info_index++;
            continue;
        }

        remove_child(*root_item, old_info_index);
    }

    while (old_info_index < static_cast<size_t>(root_item->item_count())) {
        remove_child(*root_item, old_info_index);
    }

    while (new_info_index < num_processes) {
        add_child<ProcessInfo>(*root_item, info[new_info_index++]);
    }

    free_procfs_info(info);
}

App::ModelItemInfo ProcessInfo::info(int field, int request) const {
    auto info = App::ModelItemInfo {};
    switch (field) {
        case ProcessModel::Column::Pid:
            if (request & App::ModelItemInfo::Request::Text) {
                info.set_text(format("{}", pid()));
            }
            break;
        case ProcessModel::Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(name());
            break;
        case ProcessModel::Column::Memory:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(format("{}", resident_memory()));
            if (request & App::ModelItemInfo::Request::TextAlign)
                info.set_text_align(TextAlign::CenterRight);
            break;
        case ProcessModel::Column::Priority:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(format("{}", priority()));
            if (request & App::ModelItemInfo::Request::TextAlign)
                info.set_text_align(TextAlign::CenterRight);
            break;
        case ProcessModel::Column::RunningTime: {
            if (request & App::ModelItemInfo::Request::Text) {
                double seconds = running_time().tv_sec + running_time().tv_nsec / 1000000000.0;
                long int_seconds = (int) seconds;
                seconds -= int_seconds;
                seconds *= 100;

                long minutes = int_seconds / 60;
                int_seconds %= 60;

                info.set_text(format("{}:{:02}.{:02}", minutes, int_seconds, static_cast<int>(seconds)));
            }

            if (request & App::ModelItemInfo::Request::TextAlign)
                info.set_text_align(TextAlign::CenterRight);
            break;
        }
        default:
            break;
    }
    return info;
}

App::ModelItemInfo ProcessModel::header_info(int field, int request) const {
    auto info = App::ModelItemInfo {};
    switch (field) {
        case Column::Pid:
            info.set_text("PID");
            break;
        case Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Name");
            break;
        case Column::Memory:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Memory");
            break;
        case Column::Priority:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Priority");
            break;
        case Column::RunningTime:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Running Time");
            break;
        default:
            break;
    }
    return info;
}
