#pragma once

#include <gui/widget.h>

class ProcessModel;

class ProcessTab final : public GUI::Widget {
    APP_WIDGET(GUI::Widget, ProcessTab)

public:
    ProcessTab(SharedPtr<ProcessModel> model);
    virtual void did_attach() override;
    virtual ~ProcessTab() override;

private:
    SharedPtr<ProcessModel> m_model;
};
