#pragma once

#include "document.h"

class Command {
public:
    Command(Document& document);
    virtual ~Command();

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    virtual bool execute() = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;

private:
    Document& m_document;
};

class DeltaBackedCommand : public Command {
public:
    DeltaBackedCommand(Document& document);
    virtual ~DeltaBackedCommand() override;

    virtual void redo() override;

    const Document::StateSnapshot& state_snapshot() const { return m_snapshot; }

    const String& selection_text() const { return m_selection_text; }

private:
    Document::StateSnapshot m_snapshot;
    String m_selection_text;
};

class SnapshotBackedCommand : public Command {
public:
    SnapshotBackedCommand(Document& document);
    virtual ~SnapshotBackedCommand() override;

    virtual void undo() override;
    virtual void redo() override;

    const Document::Snapshot& snapshot() const { return m_snapshot; }

private:
    Document::Snapshot m_snapshot;
};

class InsertCommand final : public DeltaBackedCommand {
public:
    InsertCommand(Document& document, String string);
    virtual ~InsertCommand();

    virtual bool execute() override;
    virtual void undo() override;

    void split_line_execute();
    void split_line_undo();

    void do_insert(char c);

private:
    String m_text;
};

class DeleteCommand final : public SnapshotBackedCommand {
public:
    DeleteCommand(Document& document, DeleteCharMode mode);
    virtual ~DeleteCommand();

    virtual bool execute() override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
};