#include <app/flex_layout_engine.h>
#include <app/table_view.h>

#include "process_model.h"
#include "process_tab.h"

ProcessTab::ProcessTab(SharedPtr<ProcessModel> model) : m_model(move(model)) {}

void ProcessTab::did_attach() {
    auto& layout = set_layout_engine<App::VerticalFlexLayoutEngine>();
    layout.set_margins({ 0, 0, 0, 0 });

    auto& tabel = layout.add<App::TableView>();
    tabel.set_model(m_model);

    Widget::did_attach();
}

ProcessTab::~ProcessTab() {}
