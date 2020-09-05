#pragma once

#include <app/widget.h>

class ProcessModel;

class ProcessTab final : public App::Widget {
    APP_OBJECT(ProcessTab)

public:
    ProcessTab(SharedPtr<ProcessModel> model);
    virtual void initialize() override;
    virtual ~ProcessTab() override;

private:
    SharedPtr<ProcessModel> m_model;
};
