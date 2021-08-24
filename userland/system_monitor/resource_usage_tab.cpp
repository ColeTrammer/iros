#include <app/flex_layout_engine.h>
#include <app/text_label.h>

#include "process_model.h"
#include "resource_usage_tab.h"

ResourceUsageTab::ResourceUsageTab(SharedPtr<ProcessModel> model) : m_model(move(model)) {}

void ResourceUsageTab::initialize() {
    auto& layout = set_layout_engine<App::VerticalFlexLayoutEngine>();
    layout.set_margins({ 0, 0, 0, 0 });
    layout.set_spacing(0);
    m_cpu_label = layout.add<App::TextLabel>("CPU: 0%").shared_from_this();
    m_memory_label = layout.add<App::TextLabel>("Memory: 0 / 0 (0%)").shared_from_this();

    m_model->register_client(this);

    Widget::initialize();
}

ResourceUsageTab::~ResourceUsageTab() {
    m_model->unregister_client(this);
}

void ResourceUsageTab::update_display() {
    auto& info = m_model->global_process_info();

    double cpu_percent = (double) (info.delta_user_ticks() + info.delta_kernel_ticks()) / (double) info.delta_total_ticks() * 100.0;
    m_cpu_label->set_text(String::format("CPU: %.2f%%", cpu_percent));

    double memory_percent = (double) info.allocated_memory() / (double) info.total_memory() * 100.0;
    m_memory_label->set_text(String::format("Memory: %lu / %lu (%.2f%%)", info.allocated_memory(), info.total_memory(), memory_percent));
}
