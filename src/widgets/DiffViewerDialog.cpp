#include "DiffViewerDialog.h"
#include <QFont>
#include <QTextCharFormat>
#include <QTextCursor>

DiffViewerDialog::DiffViewerDialog(const QString& filename, const QString& diffContent, QWidget *parent)
    : QDialog(parent)
{
    setupUi(filename, diffContent);
}

void DiffViewerDialog::setupUi(const QString& filename, const QString& diffContent)
{
    setWindowTitle(QString("Diff: %1").arg(filename));
    setModal(true);
    setMinimumSize(600, 400);
    resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Text edit for diff content
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);

    // Set monospace font
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(10);
    m_textEdit->setFont(font);

    // Set content and apply highlighting
    m_textEdit->setPlainText(diffContent);
    applyDiffHighlighting();

    // Close button
    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addWidget(m_textEdit);
    layout->addWidget(m_closeButton);

    setLayout(layout);
}

void DiffViewerDialog::applyDiffHighlighting()
{
    QTextCursor cursor(m_textEdit->document());

    // Define formats
    QTextCharFormat addedFormat;
    addedFormat.setForeground(QColor(0, 180, 0));  // Green

    QTextCharFormat removedFormat;
    removedFormat.setForeground(QColor(220, 0, 0));  // Red

    QTextCharFormat headerFormat;
    headerFormat.setForeground(QColor(100, 100, 200));  // Blue-gray

    QTextCharFormat hunkFormat;
    hunkFormat.setForeground(QColor(128, 128, 128));  // Gray

    // Process each line
    cursor.movePosition(QTextCursor::Start);

    while (!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

        QString line = cursor.selectedText();

        if (line.startsWith("+") && !line.startsWith("+++")) {
            cursor.setCharFormat(addedFormat);
        } else if (line.startsWith("-") && !line.startsWith("---")) {
            cursor.setCharFormat(removedFormat);
        } else if (line.startsWith("@@")) {
            cursor.setCharFormat(hunkFormat);
        } else if (line.startsWith("diff ") || line.startsWith("index ") ||
                   line.startsWith("---") || line.startsWith("+++")) {
            cursor.setCharFormat(headerFormat);
        }

        cursor.movePosition(QTextCursor::NextBlock);
    }

    // Move cursor back to start
    cursor.movePosition(QTextCursor::Start);
    m_textEdit->setTextCursor(cursor);
}
