#pragma once

#include <edit/document.h>
#include <edit/text_index.h>

namespace Edit {
class Command {
public:
    Command(Document& document);
    virtual ~Command();

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    virtual bool execute(Display& display) = 0;
    virtual void undo(Display& display) = 0;
    virtual void redo(Display& display) = 0;

    void set_restore_selections(bool b) { m_restore_selections = b; }
    bool restore_selections() const { return m_restore_selections; }

private:
    Document& m_document;
    bool m_restore_selections { false };
};

class DeltaBackedCommand : public Command {
public:
    DeltaBackedCommand(Document& document, Display& display);
    virtual ~DeltaBackedCommand() override;

    virtual void redo(Display& display) final;
    virtual bool execute(Display& display) final;
    virtual void undo(Display& display) final;

    virtual bool do_execute(MultiCursor& cursors) = 0;
    virtual void do_undo(MultiCursor& cursors) = 0;

    const Document::StateSnapshot& start_snapshot() const { return m_start_snapshot; }
    const Document::StateSnapshot& end_snapshot() const { return m_end_snapshot; }

    const String& selection_text(int index) const { return m_selection_texts[index]; }

private:
    Document::StateSnapshot m_start_snapshot;
    Document::StateSnapshot m_end_snapshot;
    Vector<String> m_selection_texts;
};

class SnapshotBackedCommand : public Command {
public:
    SnapshotBackedCommand(Document& document, Display& display);
    virtual ~SnapshotBackedCommand() override;

    virtual void undo(Display& display) override;
    virtual void redo(Display& display) override;

    const Document::Snapshot& snapshot() const { return m_snapshot; }

private:
    Document::Snapshot m_snapshot;
};

class InsertCommand final : public DeltaBackedCommand {
public:
    InsertCommand(Document& document, Display& display, String string);
    virtual ~InsertCommand();

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

    static void do_insert(Document& document, MultiCursor& cursors, int cursor_index, char c);
    static void do_insert(Document& document, MultiCursor& cursors, int cursor_index, const String& string);

private:
    String m_text;
};

class DeleteCommand final : public DeltaBackedCommand {
public:
    DeleteCommand(Document& document, Display& display);
    virtual ~DeleteCommand();

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;
};

class DeleteLineCommand final : public DeltaBackedCommand {
public:
    DeleteLineCommand(Document& document, Display& display);
    virtual ~DeleteLineCommand();

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    Vector<Line> m_saved_lines;
    bool m_document_was_empty { false };
};

class InsertLineCommand final : public DeltaBackedCommand {
public:
    InsertLineCommand(Document& document, Display& display, String text);
    virtual ~InsertLineCommand() override;

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    String m_text;
};

class SwapLinesCommand final : public DeltaBackedCommand {
public:
    SwapLinesCommand(Document& document, Display& display, SwapDirection direction);
    virtual ~SwapLinesCommand() override;

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    bool do_swap(Cursor& cursor, SwapDirection direction);

    SwapDirection m_direction { SwapDirection::Down };
};
}
