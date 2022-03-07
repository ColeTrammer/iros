#pragma once

#include <app/widget.h>

class ProcessModel;

class ProcessTab final : public App::Widget {
    APP_WIDGET(App::Widget, ProcessTab)

public:
    ProcessTab(SharedPtr<ProcessModel> model);
    virtual void did_attach() override;
    virtual ~ProcessTab() override;

private:
    SharedPtr<ProcessModel> m_model;
};
