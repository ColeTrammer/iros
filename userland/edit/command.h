#pragma once

#include "document.h"

class Command {
public:
    Command(Document& document);
    virtual ~Command();

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    virtual void execute() = 0;
    virtual void undo() = 0;

private:
    Document& m_document;
};

class SnapshotBackedCommand : public Command {
public:
    SnapshotBackedCommand(Document& document);
    virtual ~SnapshotBackedCommand() override;

    virtual void undo() override;

    const Document::Snapshot& snapshot() const { return m_snapshot; }

private:
    Document::Snapshot m_snapshot;
};

class InsertCommand final : public SnapshotBackedCommand {
public:
    InsertCommand(Document& document, char c);
    virtual ~InsertCommand();

    virtual void execute() override;

private:
    char m_char { 0 };
};

class DeleteCommand final : public SnapshotBackedCommand {
public:
    DeleteCommand(Document& document, DeleteCharMode mode);
    virtual ~DeleteCommand();

    virtual void execute() override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
};

class SplitLineCommand final : public SnapshotBackedCommand {
public:
    SplitLineCommand(Document& document);
    virtual ~SplitLineCommand();

    virtual void execute() override;
};