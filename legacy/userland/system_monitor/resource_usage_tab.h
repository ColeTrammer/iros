#pragma once

#include <gui/widget.h>

class ProcessModel;

class ResourceUsageTab final : public GUI::Widget {
    APP_WIDGET(GUI::Widget, ResourceUsageTab)

public:
    explicit ResourceUsageTab(SharedPtr<ProcessModel> model);
    virtual void did_attach() override;
    virtual ~ResourceUsageTab() override;

    void update_display();

private:
    SharedPtr<ProcessModel> m_model;
    SharedPtr<GUI::TextLabel> m_cpu_label;
    SharedPtr<GUI::TextLabel> m_memory_label;
};
