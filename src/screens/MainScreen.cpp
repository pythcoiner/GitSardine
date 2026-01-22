#include "MainScreen.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QPixmap>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "icons/icons.h"

MainScreen::MainScreen(QWidget *parent)
    : QWidget(parent)
    , m_folderModel(nullptr)
    , m_gitWorker(nullptr)
    , m_isExtended(false)
    , m_extend(190)
    , m_nextRequestId(1)
    , m_buttonsEnabled(false)
{
    setupUi();
    setupConnections();
    applyStyle();
    disableButtons();
    lockButtons();
}

MainScreen::~MainScreen()
{
}

void MainScreen::setupUi()
{
    // Main layout - using absolute positioning to match spec
    setFixedSize(290, 700);
    setMouseTracking(true);

    // Create all buttons first
    createButtons();

    // Repo tree
    m_repoTree = new RepoTreeWidget(this);
    m_repoTree->setGeometry(10, 40, 271, 231);

    // Branch selector
    m_branchSelector = new BranchSelector(this);
    m_branchSelector->setGeometry(70, 280, 150, 25);

    // Repo label
    m_repoLabel = new QLabel(this);
    m_repoLabel->setGeometry(10, 310, 271, 20);
    m_repoLabel->setAlignment(Qt::AlignCenter);

    // Changes tree
    m_changesTree = new ChangesTreeWidget(this);
    m_changesTree->setGeometry(10, 331, 271, 231);

    // Message input
    m_messageInput = new QLineEdit(this);
    m_messageInput->setGeometry(10, 570, 271, 25);
    m_messageInput->setPlaceholderText("Commit msg or new branch name");

    // From label for merge
    QLabel* fromLabel = new QLabel("From", this);
    fromLabel->setGeometry(110, 631, 41, 20);

    // Merge selector
    m_mergeSelector = new QComboBox(this);
    m_mergeSelector->setGeometry(150, 630, 131, 25);

    // Status bar
    m_statusBar = new StatusBar(this);
    m_statusBar->setGeometry(10, 660, 271, 31);

    // Setup timers
    m_spinnerTimer = new QTimer(this);
    m_spinnerTimer->setInterval(200);

    m_changesUpdateTimer = new QTimer(this);
    m_changesUpdateTimer->setInterval(2000);
}

void MainScreen::createButtons()
{
    // Update tree button
    m_updateTreeBtn = new QPushButton(this);
    m_updateTreeBtn->setGeometry(10, 10, 25, 25);
    m_updateTreeBtn->setIcon(loadIcon(icon_arrow_circle_315_png, icon_arrow_circle_315_png_len));
    m_updateTreeBtn->setToolTip("Update repositories status");

    // Extend button
    m_extendBtn = new QPushButton(this);
    m_extendBtn->setGeometry(257, 10, 25, 25);
    m_extendBtn->setIcon(loadIcon(icon_navigation_000_button_white_png, icon_navigation_000_button_white_png_len));

    // Delete branch button
    m_deleteBtn = new QPushButton(this);
    m_deleteBtn->setGeometry(10, 280, 25, 25);
    m_deleteBtn->setIcon(loadIcon(icon_cross_button_png, icon_cross_button_png_len));
    m_deleteBtn->setToolTip("Delete current branch (Press ctrl key for unlock)");
    m_deleteBtn->setEnabled(false);

    // Update changes button
    m_updateBtn = new QPushButton(this);
    m_updateBtn->setGeometry(40, 280, 25, 25);
    m_updateBtn->setIcon(loadIcon(icon_arrow_circle_315_png, icon_arrow_circle_315_png_len));
    m_updateBtn->setToolTip("Update changes");

    // Pull button
    m_pullBtn = new QPushButton(this);
    m_pullBtn->setGeometry(225, 280, 25, 25);
    m_pullBtn->setIcon(loadIcon(icon_arrow_skip_270_png, icon_arrow_skip_270_png_len));
    m_pullBtn->setToolTip("Git pull from origin");

    // Push button
    m_pushBtn = new QPushButton(this);
    m_pushBtn->setGeometry(257, 280, 25, 25);
    m_pushBtn->setIcon(loadIcon(icon_arrow_skip_090_png, icon_arrow_skip_090_png_len));
    m_pushBtn->setToolTip("Git push to origin");

    // Delete file button
    m_deleteFileBtn = new QPushButton(this);
    m_deleteFileBtn->setGeometry(10, 600, 25, 25);
    m_deleteFileBtn->setIcon(loadIcon(icon_cross_png, icon_cross_png_len));
    m_deleteFileBtn->setToolTip("Delete selected file (Press ctrl key for unlock)");
    m_deleteFileBtn->setEnabled(false);

    // Clean/Restore button
    m_cleanBtn = new QPushButton(this);
    m_cleanBtn->setGeometry(40, 600, 25, 25);
    m_cleanBtn->setIcon(loadIcon(icon_arrow_curve_180_left_png, icon_arrow_curve_180_left_png_len));
    m_cleanBtn->setToolTip("Git restore . (restore files to last commit state Press ctrl key for unlock)");
    m_cleanBtn->setEnabled(false);

    // Reset button
    m_resetBtn = new QPushButton(this);
    m_resetBtn->setGeometry(70, 600, 25, 25);
    m_resetBtn->setIcon(loadIcon(icon_disk_minus_png, icon_disk_minus_png_len));
    m_resetBtn->setToolTip("Git reset (cancel staged changes)");

    // Commit button
    m_commitBtn = new QPushButton("Commit", this);
    m_commitBtn->setGeometry(100, 600, 61, 25);
    m_commitBtn->setToolTip("Add & Commit file to branch");

    // Commit+Push button
    m_commitPushBtn = new QPushButton("C + Push", this);
    m_commitPushBtn->setGeometry(165, 600, 61, 25);
    m_commitPushBtn->setToolTip("Add & Commit file to branch + Push to origin");

    // Ignore button
    m_ignoreBtn = new QPushButton("Ignore", this);
    m_ignoreBtn->setGeometry(230, 600, 51, 25);
    m_ignoreBtn->setToolTip("Add this file to .gitignore file");

    // Merge button
    m_mergeBtn = new QPushButton("Merge", this);
    m_mergeBtn->setGeometry(10, 630, 91, 25);
    m_mergeBtn->setToolTip("Merge selected branch into current branch");
}

void MainScreen::setupConnections()
{
    // Button connections
    connect(m_updateTreeBtn, &QPushButton::clicked, this, &MainScreen::onUpdateTreeClicked);
    connect(m_extendBtn, &QPushButton::clicked, this, &MainScreen::onExtendClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainScreen::onDeleteBranchClicked);
    connect(m_updateBtn, &QPushButton::clicked, this, &MainScreen::onUpdateChangesClicked);
    connect(m_pullBtn, &QPushButton::clicked, this, &MainScreen::onPullClicked);
    connect(m_pushBtn, &QPushButton::clicked, this, &MainScreen::onPushClicked);
    connect(m_deleteFileBtn, &QPushButton::clicked, this, &MainScreen::onDeleteFileClicked);
    connect(m_cleanBtn, &QPushButton::clicked, this, &MainScreen::onRestoreClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainScreen::onResetClicked);
    connect(m_commitBtn, &QPushButton::clicked, this, &MainScreen::onCommitClicked);
    connect(m_commitPushBtn, &QPushButton::clicked, this, &MainScreen::onCommitPushClicked);
    connect(m_ignoreBtn, &QPushButton::clicked, this, &MainScreen::onIgnoreClicked);
    connect(m_mergeBtn, &QPushButton::clicked, this, &MainScreen::onMergeClicked);

    // Tree connections
    connect(m_repoTree, &RepoTreeWidget::repoSelected, this, &MainScreen::onRepoSelected);
    connect(m_changesTree, &ChangesTreeWidget::filesChecked, this, &MainScreen::onFilesChecked);
    connect(m_changesTree, &ChangesTreeWidget::diffRequested, this, &MainScreen::onDiffRequested);

    // Branch selector connections
    connect(m_branchSelector, &BranchSelector::branchChanged, this, &MainScreen::onBranchChanged);
    connect(m_branchSelector, &BranchSelector::newBranchRequested, this, &MainScreen::onNewBranchRequested);
}

void MainScreen::applyStyle()
{
    // Dark theme is applied globally in main.cpp
}

void MainScreen::setFolderModel(FolderTreeModel* model)
{
    m_folderModel = model;
    m_repoTree->setFolderModel(model);
    m_repoTree->expandAllItems();
}

void MainScreen::setGitWorker(GitWorker* worker)
{
    m_gitWorker = worker;
    if (worker) {
        connect(worker, &GitWorker::taskCompleted, this, &MainScreen::onGitTaskCompleted);
        connect(worker, &GitWorker::progressUpdate, this, &MainScreen::onProgressUpdate);
    }
}

QIcon MainScreen::loadIcon(const unsigned char* data, unsigned int len)
{
    QPixmap pixmap;
    pixmap.loadFromData(data, len, "PNG");
    return QIcon(pixmap);
}

int MainScreen::generateRequestId()
{
    return m_nextRequestId++;
}

void MainScreen::setLabel(const QString& message, const QString& tooltip)
{
    m_statusBar->setStatus(message, tooltip);
}

void MainScreen::enableButtons()
{
    m_buttonsEnabled = true;
    m_commitBtn->setEnabled(true);
    m_commitPushBtn->setEnabled(true);
    m_ignoreBtn->setEnabled(true);
    m_mergeBtn->setEnabled(true);
    m_pullBtn->setEnabled(true);
    m_pushBtn->setEnabled(true);
    m_updateBtn->setEnabled(true);
    m_resetBtn->setEnabled(true);
}

void MainScreen::disableButtons()
{
    m_buttonsEnabled = false;
    m_commitBtn->setEnabled(false);
    m_commitPushBtn->setEnabled(false);
    m_ignoreBtn->setEnabled(false);
    m_mergeBtn->setEnabled(false);
    m_pullBtn->setEnabled(false);
    m_pushBtn->setEnabled(false);
    m_updateBtn->setEnabled(false);
    m_resetBtn->setEnabled(false);
}

void MainScreen::lockButtons()
{
    m_deleteBtn->setEnabled(false);
    m_cleanBtn->setEnabled(false);
    m_deleteFileBtn->setEnabled(false);
}

void MainScreen::unlockButtons()
{
    if (m_buttonsEnabled) {
        m_deleteBtn->setEnabled(true);
        m_cleanBtn->setEnabled(true);
        m_deleteFileBtn->setEnabled(true);
    }
}

void MainScreen::updateBranchVisibility()
{
    bool isMainBranch = (m_currentBranch == "master" || m_currentBranch == "main");
    m_commitBtn->setVisible(!isMainBranch);
    m_commitPushBtn->setVisible(!isMainBranch);
    m_ignoreBtn->setVisible(!isMainBranch);
}

void MainScreen::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control) {
        unlockButtons();
    }
    QWidget::keyPressEvent(event);
}

void MainScreen::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control) {
        lockButtons();
    }
    QWidget::keyReleaseEvent(event);
}

void MainScreen::onUpdateTreeClicked()
{
    updateAllRepoStatus();
}

void MainScreen::updateAllRepoStatus()
{
    if (!m_gitWorker || !m_folderModel) return;

    QStringList repoPaths = m_folderModel->getAllRepoPaths();
    if (repoPaths.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::CheckAllStatus;
    req.args = repoPaths;
    req.requestId = generateRequestId();

    m_statusBar->showProgress(true);
    emit gitTaskRequested(req);
    m_gitWorker->queueTask(req);
}

void MainScreen::onExtendClicked()
{
    if (!m_isExtended) {
        m_isExtended = true;
        m_extendBtn->setIcon(loadIcon(icon_navigation_180_button_white_png, icon_navigation_180_button_white_png_len));
        setFixedWidth(width() + m_extend);
        m_repoTree->setFixedWidth(m_repoTree->width() + m_extend);
        m_changesTree->setFixedWidth(m_changesTree->width() + m_extend);
        m_extendBtn->move(m_extendBtn->x() + m_extend, m_extendBtn->y());
    } else {
        m_isExtended = false;
        m_extendBtn->setIcon(loadIcon(icon_navigation_000_button_white_png, icon_navigation_000_button_white_png_len));
        setFixedWidth(width() - m_extend);
        m_repoTree->setFixedWidth(m_repoTree->width() - m_extend);
        m_changesTree->setFixedWidth(m_changesTree->width() - m_extend);
        m_extendBtn->move(m_extendBtn->x() - m_extend, m_extendBtn->y());
    }
}

void MainScreen::onRepoSelected(const QString& path)
{
    m_currentRepoPath = path;
    enableButtons();

    // Update repo label
    QString repoName = QDir(path).dirName();
    m_repoLabel->setText(QString("%1 : ...").arg(repoName));

    // Request branch list
    if (m_gitWorker) {
        GitTaskRequest req;
        req.task = GitTask::GetBranches;
        req.repoPath = path;
        req.requestId = generateRequestId();
        m_gitWorker->queueTask(req);

        // Request changes list
        GitTaskRequest changesReq;
        changesReq.task = GitTask::GetChanges;
        changesReq.repoPath = path;
        changesReq.requestId = generateRequestId();
        m_gitWorker->queueTask(changesReq);
    }
}

void MainScreen::onBranchChanged(const QString& branch)
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Checkout;
    req.repoPath = m_currentRepoPath;
    req.args << branch;
    req.requestId = generateRequestId();

    setLabel(QString("Switching to branch: %1").arg(branch));
    m_gitWorker->queueTask(req);
}

void MainScreen::onNewBranchRequested(const QString& name)
{
    QString branchName = m_messageInput->text().trimmed();

    if (branchName.isEmpty()) {
        QMessageBox::information(this, "Branch Name Required", "Please enter a branch name.");
        return;
    }

    if (branchName.contains(' ')) {
        QMessageBox::information(this, "Invalid Branch Name",
            "Spaces are not allowed in branch names. Use '_' or '-' instead.");
        return;
    }

    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::CreateBranch;
    req.repoPath = m_currentRepoPath;
    req.args << branchName;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onFilesChecked(const QStringList& files)
{
    // Update UI based on selected files
}

void MainScreen::onDeleteBranchClicked()
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    if (m_currentBranch == "master" || m_currentBranch == "main") {
        setLabel("Cannot delete master/main");
        return;
    }

    GitTaskRequest req;
    req.task = GitTask::DeleteBranch;
    req.repoPath = m_currentRepoPath;
    req.args << m_currentBranch;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onUpdateChangesClicked()
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::GetChanges;
    req.repoPath = m_currentRepoPath;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onPullClicked()
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Pull;
    req.repoPath = m_currentRepoPath;
    req.requestId = generateRequestId();

    setLabel(QString("Pulling branch: %1").arg(m_currentBranch));
    m_statusBar->showProgress(true);
    m_gitWorker->queueTask(req);
}

void MainScreen::onPushClicked()
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Push;
    req.repoPath = m_currentRepoPath;
    req.requestId = generateRequestId();

    setLabel(QString("Pushing branch: %1").arg(m_currentBranch));
    m_statusBar->showProgress(true);
    m_gitWorker->queueTask(req);
}

void MainScreen::onCommitClicked()
{
    QString message = m_messageInput->text().trimmed();
    if (message.isEmpty()) {
        QMessageBox::information(this, "Commit Message Required", "Please enter a commit message.");
        return;
    }

    QStringList files = m_changesTree->getCheckedFiles();

    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Commit;
    req.repoPath = m_currentRepoPath;
    req.args << message;
    req.args.append(files);
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onCommitPushClicked()
{
    // First commit, then push on success (handled in onGitTaskCompleted)
    onCommitClicked();
}

void MainScreen::onIgnoreClicked()
{
    QStringList files = m_changesTree->getCheckedFiles();
    if (files.isEmpty()) return;

    // Add files to .gitignore
    QString gitignorePath = QDir(m_currentRepoPath).filePath(".gitignore");
    QFile file(gitignorePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString& f : files) {
            out << f << "\n";
        }
        file.close();
        setLabel("Files added to .gitignore");
        onUpdateChangesClicked();
    }
}

void MainScreen::onMergeClicked()
{
    QString sourceBranch = m_mergeSelector->currentText();
    if (sourceBranch.isEmpty()) {
        setLabel("No branch to merge from");
        return;
    }

    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Merge;
    req.repoPath = m_currentRepoPath;
    req.args << sourceBranch;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onResetClicked()
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Reset;
    req.repoPath = m_currentRepoPath;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onRestoreClicked()
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::Restore;
    req.repoPath = m_currentRepoPath;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onDeleteFileClicked()
{
    QStringList files = m_changesTree->getCheckedFiles();
    for (const QString& file : files) {
        QString fullPath = QDir(m_currentRepoPath).filePath(file);
        QFile::remove(fullPath);
    }
    onUpdateChangesClicked();
}

void MainScreen::onDiffRequested(const QString& file)
{
    if (!m_gitWorker || m_currentRepoPath.isEmpty()) return;

    GitTaskRequest req;
    req.task = GitTask::GetDiff;
    req.repoPath = m_currentRepoPath;
    req.args << file;
    req.requestId = generateRequestId();

    m_gitWorker->queueTask(req);
}

void MainScreen::onProgressUpdate(int requestId, int percent, QString status)
{
    m_statusBar->setProgress(percent);
}

void MainScreen::onGitTaskCompleted(GitTaskResult result)
{
    m_statusBar->showProgress(false);

    if (!result.success) {
        setLabel(result.message);
        return;
    }

    // Refresh branch list after branch-related operations
    if (result.task == GitTask::CreateBranch ||
        result.task == GitTask::DeleteBranch ||
        result.task == GitTask::Checkout ||
        result.task == GitTask::Merge) {
        // Request updated branch list
        if (!m_currentRepoPath.isEmpty() && m_gitWorker) {
            GitTaskRequest req;
            req.task = GitTask::GetBranches;
            req.repoPath = m_currentRepoPath;
            m_gitWorker->queueTask(req);
        }
    }

    // Refresh changes list after file-related operations
    if (result.task == GitTask::Commit ||
        result.task == GitTask::Reset ||
        result.task == GitTask::Restore ||
        result.task == GitTask::Checkout ||
        result.task == GitTask::Pull ||
        result.task == GitTask::Merge ||
        result.task == GitTask::StashPop) {
        // Request updated changes list
        if (!m_currentRepoPath.isEmpty() && m_gitWorker) {
            GitTaskRequest req;
            req.task = GitTask::GetChanges;
            req.repoPath = m_currentRepoPath;
            m_gitWorker->queueTask(req);
        }
    }

    // Handle specific task results
    if (result.data.type() == QVariant::Map) {
        QVariantMap data = result.data.toMap();

        // Handle GetBranches result
        if (data.contains("local") && data.contains("remote")) {
            QStringList local = data["local"].toStringList();
            QStringList remote = data["remote"].toStringList();
            QString current = data["current"].toString();

            m_currentBranch = current;
            m_branchSelector->setBranches(local, remote, current);

            // Update merge selector
            m_mergeSelector->clear();
            for (const QString& b : local) {
                if (b != current) {
                    m_mergeSelector->addItem(b);
                }
            }

            // Update repo label
            QString repoName = QDir(m_currentRepoPath).dirName();
            m_repoLabel->setText(QString("%1 : %2").arg(repoName, current));

            updateBranchVisibility();
        }

        // Handle GetChanges result
        if (data.contains("modified") && data.contains("staged")) {
            QStringList modified = data["modified"].toStringList();
            QStringList staged = data["staged"].toStringList();

            QList<FileChange> unstagedChanges;
            for (const QString& f : modified) {
                unstagedChanges.append(FileChange(f, FileStatus::Modified, false));
            }

            QList<FileChange> stagedChanges;
            for (const QString& f : staged) {
                stagedChanges.append(FileChange(f, FileStatus::Staged, true));
            }

            m_changesTree->setChanges(unstagedChanges, stagedChanges);
        }

        // Handle CheckAllStatus result
        if (data.contains("path") && data.contains("status")) {
            QString path = data["path"].toString();
            QVariantMap statusMap = data["status"].toMap();
            RepoStatus status;
            status.needsPull = statusMap["needsPull"].toBool();
            status.needsPush = statusMap["needsPush"].toBool();
            status.needsCommit = statusMap["needsCommit"].toBool();
            status.hasError = statusMap["hasError"].toBool();

            if (m_folderModel) {
                m_folderModel->updateRepoStatus(path, status);
            }
        }
    }

    // Handle list of status updates
    if (result.data.type() == QVariant::List) {
        QVariantList statusList = result.data.toList();
        for (const QVariant& item : statusList) {
            QVariantMap repoData = item.toMap();
            QString path = repoData["path"].toString();
            QVariantMap statusMap = repoData["status"].toMap();

            RepoStatus status;
            status.needsPull = statusMap["needsPull"].toBool();
            status.needsPush = statusMap["needsPush"].toBool();
            status.needsCommit = statusMap["needsCommit"].toBool();
            status.hasError = statusMap["hasError"].toBool();

            if (m_folderModel) {
                m_folderModel->updateRepoStatus(path, status);
            }
        }
    }

    // Handle diff result
    if (result.data.type() == QVariant::String && !result.data.toString().isEmpty()) {
        QString diffContent = result.data.toString();
        // Extract filename from the diff output or use a placeholder
        DiffViewerDialog dialog("file", diffContent, this);
        dialog.exec();
    }

    setLabel(result.message);
}
