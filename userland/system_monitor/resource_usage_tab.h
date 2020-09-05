#pragma once

#include <app/model_client.h>
#include <app/widget.h>

class ProcessModel;

namespace App {
class TextLabel;
}

class ResourceUsageTab final
    : public App::Widget
    , public App::ModelClient {
    APP_OBJECT(ResourceUsageTab)

public:
    ResourceUsageTab(SharedPtr<ProcessModel> model);
    virtual void initialize() override;
    virtual ~ResourceUsageTab() override;

    virtual void model_did_update() override { update_display(); }

    void update_display();

private:
    SharedPtr<ProcessModel> m_model;
    SharedPtr<App::TextLabel> m_cpu_label;
    SharedPtr<App::TextLabel> m_memory_label;
};
