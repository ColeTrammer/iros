#pragma once

#include <app/widget.h>

class ProcessModel;

class ResourceUsageTab final : public App::Widget {
    APP_OBJECT(ResourceUsageTab)

public:
    explicit ResourceUsageTab(SharedPtr<ProcessModel> model);
    virtual void initialize() override;
    virtual ~ResourceUsageTab() override;

    void update_display();

private:
    SharedPtr<ProcessModel> m_model;
    SharedPtr<App::TextLabel> m_cpu_label;
    SharedPtr<App::TextLabel> m_memory_label;
};
