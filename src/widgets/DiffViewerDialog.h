#ifndef DIFFVIEWERDIALOG_H
#define DIFFVIEWERDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

/**
 * DiffViewerDialog - Modal dialog for viewing file diffs
 */
class DiffViewerDialog : public QDialog {
    Q_OBJECT

public:
    DiffViewerDialog(const QString& filename, const QString& diffContent, QWidget *parent = nullptr);

private:
    QTextEdit* m_textEdit;
    QPushButton* m_closeButton;

    void setupUi(const QString& filename, const QString& diffContent);
    void applyDiffHighlighting();
};

#endif // DIFFVIEWERDIALOG_H
