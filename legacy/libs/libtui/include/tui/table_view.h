#pragma once

#include <tui/view.h>

namespace TUI {
class TableView : public View {
    APP_WIDGET(View, TableView)

public:
    TableView() {}

    virtual void render() override;
    virtual App::ModelItem* item_at_position(const Point&) override { return nullptr; }
};
}
