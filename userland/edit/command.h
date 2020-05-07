#pragma once

class Document;

class Command {
public:
    Command(Document& document);
    virtual ~Command();

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    virtual void execute() = 0;

private:
    Document& m_document;
};

class InsertCommand final : public Command {
public:
    InsertCommand(Document& document, char c);
    virtual ~InsertCommand();

    virtual void execute() override;

private:
    char m_char { 0 };
};

enum class DeleteCharMode { Backspace, Delete };

class DeleteCommand final : public Command {
public:
    DeleteCommand(Document& document, DeleteCharMode mode);
    virtual ~DeleteCommand();

    virtual void execute() override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
};

class SplitLineCommand final : public Command {
public:
    SplitLineCommand(Document& document);
    virtual ~SplitLineCommand();

    virtual void execute() override;
};