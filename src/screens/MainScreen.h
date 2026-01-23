#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTimer>

#include "widgets/RepoTreeWidget.h"
#include "widgets/ChangesTreeWidget.h"
#include "widgets/BranchSelector.h"
#include "widgets/StatusBar.h"
#include "widgets/DiffViewerDialog.h"
#include "models/FolderTreeModel.h"
#include "workers/GitWorker.h"

/**
 * MainScreen - Main application screen
 */
class MainScreen : public QWidget {
    Q_OBJECT

public:
    explicit MainScreen(QWidget *parent = nullptr);
    ~MainScreen();

    void setFolderModel(FolderTreeModel* model);
    void setGitWorker(GitWorker* worker);

signals:
    void statusChanged(const QString& message, const QString& tooltip);
    void gitTaskRequested(GitTaskRequest request);

public slots:
    void onGitTaskCompleted(GitTaskResult result);
    void onRepoSelected(const QString& path);
    void onBranchChanged(const QString& branch);
    void onFilesChecked(const QStringList& files);
    void updateAllRepoStatus();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onUpdateTreeClicked();
    void onExtendClicked();
    void onDeleteBranchClicked();
    void onUpdateChangesClicked();
    void onPullClicked();
    void onPushClicked();
    void onCommitClicked();
    void onCommitPushClicked();
    void onIgnoreClicked();
    void onMergeClicked();
    void onResetClicked();
    void onRestoreClicked();
    void onDeleteFileClicked();
    void onNewBranchRequested(const QString& name);
    void onDiffRequested(const QString& file);
    void onProgressUpdate(int requestId, int percent, QString status);

private:
    // Widgets
    RepoTreeWidget* m_repoTree;
    ChangesTreeWidget* m_changesTree;
    BranchSelector* m_branchSelector;
    StatusBar* m_statusBar;
    QLabel* m_repoLabel;
    QLineEdit* m_messageInput;
    QComboBox* m_mergeSelector;

    // Buttons
    QPushButton* m_updateTreeBtn;
    QPushButton* m_extendBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_updateBtn;
    QPushButton* m_pullBtn;
    QPushButton* m_pushBtn;
    QPushButton* m_deleteFileBtn;
    QPushButton* m_cleanBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_commitBtn;
    QPushButton* m_commitPushBtn;
    QPushButton* m_ignoreBtn;
    QPushButton* m_mergeBtn;

    // State
    FolderTreeModel* m_folderModel;
    GitWorker* m_gitWorker;
    QString m_currentRepoPath;
    QString m_currentBranch;
    bool m_isExtended;
    int m_extend;
    int m_nextRequestId;
    bool m_buttonsEnabled;
    bool m_isMasterBranch;

    // Timers
    QTimer* m_spinnerTimer;
    QTimer* m_changesUpdateTimer;

    // Setup methods
    void setupUi();
    void setupConnections();
    void applyStyle();
    void createButtons();

    // Helper methods
    void setLabel(const QString& message, const QString& tooltip = QString());
    void enableButtons();
    void disableButtons();
    void lockButtons();
    void unlockButtons();
    void updateBranchVisibility();
    int generateRequestId();

    QIcon loadIcon(const unsigned char* data, unsigned int len);
};

#endif // MAINSCREEN_H
