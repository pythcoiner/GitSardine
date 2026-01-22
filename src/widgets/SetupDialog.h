#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTimer>

class ScanProgressDialog;

/**
 * SetupDialog - First-run setup dialog
 *
 * Shown when no config file exists. Prompts user for:
 * - Username
 * - Directories to scan for git repositories
 */
class SetupDialog : public QDialog {
    Q_OBJECT

public:
    explicit SetupDialog(QWidget *parent = nullptr);

    QString username() const;
    QStringList paths() const;

private slots:
    void onAddPath();
    void onRemovePath();
    void validateInput();

private:
    QLineEdit *m_usernameInput;
    QListWidget *m_pathsList;
    QPushButton *m_addPathBtn;
    QPushButton *m_removePathBtn;
    QDialogButtonBox *m_buttonBox;
    QLabel *m_validationLabel;

    // Check if directory contains git repos (not is a repo itself)
    int countGitRepos(const QString& path, ScanProgressDialog* progress) const;
    bool isGitRepo(const QString& path) const;
};

/**
 * ScanProgressDialog - Modal dialog shown while scanning directories
 */
class ScanProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScanProgressDialog(const QString& path, QWidget *parent = nullptr);
    void incrementCount();
    int repoCount() const { return m_repoCount; }

private slots:
    void updateSpinner();

private:
    QLabel *m_spinnerLabel;
    QLabel *m_messageLabel;
    QLabel *m_countLabel;
    QTimer *m_spinnerTimer;
    int m_spinnerFrame;
    int m_repoCount;
};

#endif // SETUPDIALOG_H
