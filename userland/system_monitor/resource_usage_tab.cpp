#include <app/box_layout.h>
#include <app/text_label.h>

#include "process_model.h"
#include "resource_usage_tab.h"

ResourceUsageTab::ResourceUsageTab(SharedPtr<ProcessModel> model) : m_model(move(model)) {}

void ResourceUsageTab::initialize() {
    m_model->register_client(this);

    auto& layout = set_layout<App::VerticalBoxLayout>();
    layout.set_margins({ 0, 0, 0, 0 });
    m_cpu_label = layout.add<App::TextLabel>("CPU").shared_from_this();
}

ResourceUsageTab::~ResourceUsageTab() {
    m_model->unregister_client(this);
}

void ResourceUsageTab::model_did_update() {}
