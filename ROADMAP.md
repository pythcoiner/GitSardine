# GitSardine C++ Implementation Roadmap

## Usage Rules

1. **Managers update this file** - VIEW_MANAGER and FEATURE_MANAGER must mark items as complete `[x]` when tasks pass review
2. **Progress tracking** - PROJECT_MANAGER monitors this file to track overall progress
3. **Review required** - Items marked **REVIEW** require REVIEWER approval before marking complete
4. **Sequential phases** - Complete Phase N before starting Phase N+1

---

## Phase 1: Infrastructure

### 1.1 Project Setup
- [ ] Create `gitsardine-cpp/` directory structure
- [ ] Create `src/` directory
- [ ] Create `src/core/` directory
- [ ] Create `src/config/` directory
- [ ] Create `src/git/` directory
- [ ] Create `src/models/` directory
- [ ] Create `src/workers/` directory
- [ ] Create `src/widgets/` directory
- [ ] Create `src/screens/` directory
- [ ] Create `src/controller/` directory
- [ ] Create `resources/` directory
- [ ] Create `resources/icons/` directory
- [ ] Create `deps/` directory
- [ ] Clone qontrol as git submodule in `deps/qontrol/`

### 1.2 CMakeLists.txt
- [ ] Create `CMakeLists.txt`
- [ ] Set cmake_minimum_required (3.17)
- [ ] Set project name (gitsardine)
- [ ] Set C++17 standard
- [ ] Enable CMAKE_AUTOMOC
- [ ] Configure CMAKE_PREFIX_PATH for qt_static
- [ ] Add find_package(Qt6 COMPONENTS Core Gui Widgets)
- [ ] Add find_package(PkgConfig)
- [ ] Add pkg_check_modules for libgit2
- [ ] Add add_subdirectory for qontrol
- [ ] Define SOURCES list (empty initially)
- [ ] Add add_executable(gitsardine)
- [ ] Add target_include_directories
- [ ] Add target_link_libraries (qontrol, Qt6, libgit2)
- [ ] **REVIEW CMakeLists.txt**
- [ ] Fix any issues from review

### 1.3 Result.h (Error Handling)
- [ ] Create `src/core/Result.h`
- [ ] Define template class Result<T, E>
- [ ] Implement static Ok(T value) method
- [ ] Implement static Err(E error) method
- [ ] Implement isOk() const method
- [ ] Implement isErr() const method
- [ ] Implement value() const method
- [ ] Implement error() const method
- [ ] Implement map() template method
- [ ] Implement mapErr() template method
- [ ] Define VoidResult type alias
- [ ] Add Result.h to CMakeLists SOURCES
- [ ] **REVIEW Result.h**
- [ ] Fix any issues from review

### 1.4 Config System
- [ ] Create `src/config/Config.h`
- [ ] Define Config class
- [ ] Declare paths (QStringList) member
- [ ] Declare user (QString) member
- [ ] Declare extend (int) member
- [ ] Declare ignore (QVariantList) member
- [ ] Declare static getConfigPath() method
- [ ] Declare load() method returning Result
- [ ] Declare save() method returning Result
- [ ] Declare createDefault() method
- [ ] Create `src/config/Config.cpp`
- [ ] Implement getConfigPath() with platform detection
  - [ ] Linux: ~/.config/gitsardine/config.json
  - [ ] Windows: %APPDATA%/GitSardine/config.json
  - [ ] macOS: ~/Library/Application Support/GitSardine/config.json
- [ ] Implement load() using QJsonDocument
  - [ ] Check if file exists
  - [ ] Read file contents
  - [ ] Parse JSON
  - [ ] Extract "paths" array
  - [ ] Extract "user" string
  - [ ] Extract "extend" number (default 190)
  - [ ] Extract "ignore" array
  - [ ] Return Result with error if parsing fails
- [ ] Implement save() using QJsonDocument
  - [ ] Create QJsonObject
  - [ ] Add all fields
  - [ ] Write to file
  - [ ] Return Result
- [ ] Implement createDefault()
  - [ ] Create config directory if missing
  - [ ] Write default config.json
- [ ] Add Config.cpp to CMakeLists SOURCES
- [ ] **REVIEW Config.h/.cpp**
- [ ] Fix any issues from review

### 1.5 GitWorker Thread
- [ ] Create `src/workers/GitWorker.h`
- [ ] Define GitTask enum
  - [ ] CheckStatus
  - [ ] CheckAllStatus
  - [ ] Fetch
  - [ ] Pull
  - [ ] Push
  - [ ] Commit
  - [ ] Checkout
  - [ ] CreateBranch
  - [ ] DeleteBranch
  - [ ] Merge
  - [ ] Reset
  - [ ] Restore
  - [ ] Stash
  - [ ] StashPop
  - [ ] GetBranches
  - [ ] GetChanges
  - [ ] GetDiff
- [ ] Define GitTaskRequest struct
  - [ ] GitTask task
  - [ ] QString repoPath
  - [ ] QStringList args
  - [ ] int requestId
- [ ] Define GitTaskResult struct
  - [ ] int requestId
  - [ ] bool success
  - [ ] QString message
  - [ ] QVariant data
- [ ] Define GitWorker class (inherits QThread)
  - [ ] Q_OBJECT macro
  - [ ] Constructor
  - [ ] Destructor
  - [ ] Signal: taskCompleted(GitTaskResult)
  - [ ] Signal: progressUpdate(int requestId, int percent, QString status)
  - [ ] Slot: queueTask(GitTaskRequest)
  - [ ] Slot: cancelTask(int requestId)
  - [ ] Protected: run() override
  - [ ] Private: QQueue<GitTaskRequest> m_taskQueue
  - [ ] Private: QMutex m_queueMutex
  - [ ] Private: QWaitCondition m_queueCondition
  - [ ] Private: bool m_running
  - [ ] Private: QHash<QString, git_repository*> m_openRepos
  - [ ] Private: getRepo(const QString& path)
  - [ ] Private: closeAllRepos()
- [ ] Create `src/workers/GitWorker.cpp`
- [ ] Implement constructor
  - [ ] Call git_libgit2_init()
  - [ ] Initialize members
- [ ] Implement destructor
  - [ ] Set m_running = false
  - [ ] Wake thread
  - [ ] Wait for thread
  - [ ] Call closeAllRepos()
  - [ ] Call git_libgit2_shutdown()
- [ ] Implement run() - main thread loop
  - [ ] While m_running
  - [ ] Lock mutex
  - [ ] Wait on condition if queue empty
  - [ ] Dequeue task
  - [ ] Unlock mutex
  - [ ] Process task (switch on task type)
  - [ ] Emit taskCompleted
- [ ] Implement queueTask()
  - [ ] Lock mutex
  - [ ] Add to queue
  - [ ] Wake condition
  - [ ] Unlock mutex
- [ ] Implement getRepo()
  - [ ] Check if already open in m_openRepos
  - [ ] If not, open with git_repository_open
  - [ ] Cache in m_openRepos
  - [ ] Return pointer
- [ ] Implement closeAllRepos()
  - [ ] Iterate m_openRepos
  - [ ] Call git_repository_free on each
  - [ ] Clear hash
- [ ] Add GitWorker.cpp to CMakeLists SOURCES
- [ ] **REVIEW GitWorker.h/.cpp**
- [ ] Fix any issues from review

### 1.6 Icon Embedding
- [ ] Create `resources/icons/icons.h` (master include)
- [ ] For each icon in DATA_MODELS.md section 3:
  - [ ] Convert PNG to header: `xxd -i icon.png > icon.h`
  - [ ] arrow-circle-315.h
  - [ ] cross-button.h
  - [ ] arrow-skip-270.h
  - [ ] arrow-skip-090.h
  - [ ] cross.h
  - [ ] arrow-curve-180-left.h
  - [ ] disk--minus.h
  - [ ] navigation-000-button-white.h
  - [ ] navigation-180-button-white.h
  - [ ] spin_1.h, spin_2.h, spin_3.h, spin_4.h
  - [ ] exclamation-red.h
  - [ ] drive-download.h
  - [ ] drive-upload.h
  - [ ] disk--plus.h
  - [ ] document.h
  - [ ] folder-horizontal.h
  - [ ] drive.h
- [ ] Add #include for each in icons.h
- [ ] **REVIEW icon headers**
- [ ] Fix any issues from review

---

## Phase 2: Core Features + Basic UI

### 2.1 GitStatus Data Structures
- [ ] Create `src/git/GitStatus.h`
- [ ] Define FileStatus enum (Modified, Staged, Untracked, Deleted)
- [ ] Define RepoStatus struct
  - [ ] bool needsPull
  - [ ] bool needsPush
  - [ ] bool needsCommit
  - [ ] bool hasError
  - [ ] QString errorMessage
  - [ ] QString currentBranch
  - [ ] int ahead
  - [ ] int behind
- [ ] Define FileChange struct
  - [ ] QString path
  - [ ] FileStatus status
- [ ] **REVIEW GitStatus.h**
- [ ] Fix any issues from review

### 2.2 GitRepository Wrapper
- [ ] Create `src/git/GitRepository.h`
- [ ] Define GitRepository class
  - [ ] Constructor(QString path)
  - [ ] Destructor
  - [ ] isValid() const
  - [ ] path() const
  - [ ] getCurrentBranch() -> Result<QString>
  - [ ] getLocalBranches() -> Result<QStringList>
  - [ ] getRemoteBranches() -> Result<QStringList>
  - [ ] getStatus() -> Result<RepoStatus>
  - [ ] getChanges() -> Result<QList<FileChange>>
  - [ ] getCachedChanges() -> Result<QList<FileChange>>
  - [ ] Private: git_repository* m_repo
  - [ ] Private: QString m_path
- [ ] Create `src/git/GitRepository.cpp`
- [ ] Implement constructor
  - [ ] Open repository with git_repository_open
  - [ ] Store path
- [ ] Implement destructor
  - [ ] Call git_repository_free if valid
- [ ] Implement isValid()
- [ ] Implement path()
- [ ] Implement getCurrentBranch()
  - [ ] Read .git/HEAD
  - [ ] Parse ref name
  - [ ] Return branch name
- [ ] Implement getLocalBranches()
  - [ ] Use git_branch_iterator
  - [ ] Filter GIT_BRANCH_LOCAL
  - [ ] Return list
- [ ] Implement getRemoteBranches()
  - [ ] Use git_branch_iterator
  - [ ] Filter GIT_BRANCH_REMOTE
  - [ ] Return list
- [ ] Implement getStatus()
  - [ ] Fetch dry-run to check pull needed
  - [ ] Compare local/remote refs for push needed
  - [ ] Check for uncommitted changes
  - [ ] Use git_graph_ahead_behind for counts
- [ ] Implement getChanges()
  - [ ] Use git_status_foreach_ext
  - [ ] Filter with ignore patterns
  - [ ] Return file list
- [ ] Implement getCachedChanges()
  - [ ] Use git_status with INDEX flags
  - [ ] Return staged files
- [ ] Add GitRepository.cpp to CMakeLists SOURCES
- [ ] **REVIEW GitRepository.h/.cpp**
- [ ] Fix any issues from review

### 2.3 Repository Discovery
- [ ] Create `src/models/RepoModel.h`
- [ ] Define RepoModel class
  - [ ] QString name
  - [ ] QString path
  - [ ] RepoStatus status
  - [ ] bool isRepo
- [ ] Create `src/models/RepoModel.cpp`
- [ ] Create `src/models/FolderTreeModel.h`
- [ ] Define FolderTreeModel class (inherits QStandardItemModel)
  - [ ] Constructor
  - [ ] scanPaths(QStringList paths)
  - [ ] getRepoAt(QModelIndex) -> RepoModel*
  - [ ] Private: recursive scan function
- [ ] Create `src/models/FolderTreeModel.cpp`
- [ ] Implement scanPaths()
  - [ ] For each path in list
  - [ ] Recursively walk directories
  - [ ] Check for .git folder
  - [ ] Build tree structure
  - [ ] Mark repos with is_repo=true
- [ ] Implement getRepoAt()
- [ ] Add RepoModel.cpp to CMakeLists SOURCES
- [ ] Add FolderTreeModel.cpp to CMakeLists SOURCES
- [ ] **REVIEW RepoModel and FolderTreeModel**
- [ ] Fix any issues from review

### 2.4 GitWorker Task Handlers (Status)
- [ ] Add handleCheckStatus() to GitWorker
  - [ ] Get repo from path
  - [ ] Call GitRepository::getStatus()
  - [ ] Return result
- [ ] Add handleCheckAllStatus() to GitWorker
  - [ ] Accept list of paths
  - [ ] Check each repo
  - [ ] Emit progress updates
  - [ ] Return aggregated results
- [ ] Add handleGetBranches() to GitWorker
  - [ ] Get local branches
  - [ ] Get remote branches
  - [ ] Combine and return
- [ ] Add handleGetChanges() to GitWorker
  - [ ] Get unstaged changes
  - [ ] Get staged changes
  - [ ] Return both lists
- [ ] **REVIEW GitWorker task handlers**
- [ ] Fix any issues from review

### 2.5 StatusBar Widget
- [ ] Create `src/widgets/StatusBar.h`
- [ ] Define StatusBar class (inherits QWidget)
  - [ ] Q_OBJECT
  - [ ] Constructor
  - [ ] Slot: setStatus(QString message)
  - [ ] Slot: setStatus(QString message, QString tooltip)
  - [ ] Slot: showProgress(bool visible)
  - [ ] Slot: setProgress(int percent)
  - [ ] Private: QLabel* m_label
  - [ ] Private: QProgressBar* m_progress
- [ ] Create `src/widgets/StatusBar.cpp`
- [ ] Implement constructor
  - [ ] Create horizontal layout
  - [ ] Create label
  - [ ] Create progress bar (hidden by default)
  - [ ] Apply dark theme styling
- [ ] Implement setStatus(QString)
- [ ] Implement setStatus(QString, QString)
- [ ] Implement showProgress()
- [ ] Implement setProgress()
- [ ] Add StatusBar.cpp to CMakeLists SOURCES
- [ ] **REVIEW StatusBar.h/.cpp**
- [ ] Fix any issues from review

### 2.6 BranchSelector Widget
- [ ] Create `src/widgets/BranchSelector.h`
- [ ] Define BranchSelector class (inherits QWidget)
  - [ ] Q_OBJECT
  - [ ] Constructor
  - [ ] Signal: branchChanged(QString branch)
  - [ ] Signal: newBranchRequested(QString name)
  - [ ] Slot: setBranches(QStringList local, QStringList remote, QString current)
  - [ ] Slot: setEnabled(bool)
  - [ ] Private: QComboBox* m_combo
  - [ ] Private: QString m_currentBranch
  - [ ] Private slot: onComboChanged(int index)
- [ ] Create `src/widgets/BranchSelector.cpp`
- [ ] Implement constructor
  - [ ] Create combo box
  - [ ] Connect currentIndexChanged to onComboChanged
  - [ ] Apply dark theme styling
- [ ] Implement setBranches()
  - [ ] Clear combo
  - [ ] Add current branch first
  - [ ] Add other local branches
  - [ ] Add remote branches with <> markers
  - [ ] Add "--new--" option
- [ ] Implement onComboChanged()
  - [ ] If "--new--" selected, emit newBranchRequested
  - [ ] Else emit branchChanged
- [ ] Add BranchSelector.cpp to CMakeLists SOURCES
- [ ] **REVIEW BranchSelector.h/.cpp**
- [ ] Fix any issues from review

### 2.7 DiffViewerDialog
- [ ] Create `src/widgets/DiffViewerDialog.h`
- [ ] Define DiffViewerDialog class (inherits QDialog)
  - [ ] Q_OBJECT
  - [ ] Constructor(QString filename, QString diffContent, QWidget* parent)
  - [ ] Private: QTextEdit* m_textEdit
  - [ ] Private: QPushButton* m_closeButton
  - [ ] Private: void applyDiffHighlighting()
- [ ] Create `src/widgets/DiffViewerDialog.cpp`
- [ ] Implement constructor
  - [ ] Set window title "Diff: {filename}"
  - [ ] Set modal
  - [ ] Set minimum size 600x400
  - [ ] Create vertical layout
  - [ ] Create read-only QTextEdit with monospace font
  - [ ] Set diff content
  - [ ] Apply diff highlighting
  - [ ] Create Close button
  - [ ] Connect close button to accept()
- [ ] Implement applyDiffHighlighting()
  - [ ] Green for lines starting with +
  - [ ] Red for lines starting with -
  - [ ] Gray for @@ headers
- [ ] Add DiffViewerDialog.cpp to CMakeLists SOURCES
- [ ] **REVIEW DiffViewerDialog.h/.cpp**
- [ ] Fix any issues from review

---

## Phase 3: Git Operations + Tree Widgets

### 3.1 GitWorker Task Handlers (Branch Operations)
- [ ] Add handleCheckout() to GitWorker
  - [ ] Check stash count before
  - [ ] Run git stash
  - [ ] Check stash count after (to know if stash created)
  - [ ] Run git checkout
  - [ ] If stash was created, run git stash pop
  - [ ] Handle stash pop conflicts
  - [ ] Return result
- [ ] Add handleCreateBranch() to GitWorker
  - [ ] Validate name not empty
  - [ ] Validate no spaces
  - [ ] Check branch doesn't exist
  - [ ] Run git branch {name}
  - [ ] Return result
- [ ] Add handleDeleteBranch() to GitWorker
  - [ ] Validate not master/main
  - [ ] Checkout master first
  - [ ] Run git branch -D {name}
  - [ ] Return result
- [ ] **REVIEW branch operation handlers**
- [ ] Fix any issues from review

### 3.2 GitWorker Task Handlers (File Operations)
- [ ] Add handleCommit() to GitWorker
  - [ ] Accept file list and message
  - [ ] For each file: git add {file}
  - [ ] Run git commit -m "{message}"
  - [ ] Return result
- [ ] Add handleReset() to GitWorker
  - [ ] Run git reset
  - [ ] Return result
- [ ] Add handleRestore() to GitWorker
  - [ ] Run git restore .
  - [ ] Return result
- [ ] Add handleStash() to GitWorker
  - [ ] Check stash count before
  - [ ] Run git stash
  - [ ] Check stash count after
  - [ ] Return whether stash was created
- [ ] Add handleStashPop() to GitWorker
  - [ ] Run git stash pop
  - [ ] Handle conflicts
  - [ ] Return result
- [ ] **REVIEW file operation handlers**
- [ ] Fix any issues from review

### 3.3 GitWorker Task Handlers (Network Operations)
- [ ] Add handleFetch() to GitWorker
  - [ ] Get remote "origin"
  - [ ] Run git_remote_fetch
  - [ ] Handle auth errors
  - [ ] Return result
- [ ] Add handlePull() to GitWorker
  - [ ] Check for uncommitted changes first
  - [ ] If changes, return error
  - [ ] Run fetch
  - [ ] Run merge
  - [ ] Return result
- [ ] Add handlePush() to GitWorker
  - [ ] Get current branch
  - [ ] Run git_remote_push
  - [ ] Handle auth errors
  - [ ] Return result
- [ ] **REVIEW network operation handlers**
- [ ] Fix any issues from review

### 3.4 GitWorker Task Handlers (Advanced)
- [ ] Add handleMerge() to GitWorker
  - [ ] Accept source branch
  - [ ] Run git merge {source}
  - [ ] If on master/main, delete source after merge
  - [ ] Handle merge conflicts
  - [ ] Return result
- [ ] Add handleGetDiff() to GitWorker
  - [ ] Accept file path
  - [ ] Check against binary extensions blacklist
  - [ ] Run git diff {file}
  - [ ] Return diff content
- [ ] **REVIEW advanced operation handlers**
- [ ] Fix any issues from review

### 3.5 RepoTreeWidget
- [ ] Create `src/widgets/RepoTreeWidget.h`
- [ ] Define RepoTreeWidget class (inherits QTreeView)
  - [ ] Q_OBJECT
  - [ ] Constructor
  - [ ] Signal: repoSelected(QString path)
  - [ ] Signal: refreshRequested()
  - [ ] Slot: setModel(FolderTreeModel* model)
  - [ ] Slot: updateRepoStatus(QString path, RepoStatus status)
  - [ ] Slot: setSpinnerActive(bool active)
  - [ ] Private: FolderTreeModel* m_model
  - [ ] Private: QTimer* m_spinnerTimer
  - [ ] Private: int m_spinnerFrame
  - [ ] Private slot: onDoubleClicked(QModelIndex)
  - [ ] Private slot: updateSpinner()
- [ ] Create `src/widgets/RepoTreeWidget.cpp`
- [ ] Implement constructor
  - [ ] Set header hidden
  - [ ] Connect doubleClicked signal
  - [ ] Setup spinner timer (200ms)
  - [ ] Apply dark theme styling
- [ ] Implement onDoubleClicked()
  - [ ] Get item from index
  - [ ] If is_repo: emit repoSelected
  - [ ] Else: toggle expand/collapse
- [ ] Implement updateRepoStatus()
  - [ ] Find item by path
  - [ ] Update icon based on status
  - [ ] Use priority: Error > Pull > Push > Commit > Clean
- [ ] Implement setSpinnerActive()
  - [ ] Start/stop spinner timer
- [ ] Implement updateSpinner()
  - [ ] Cycle through spin/1-4.png
  - [ ] Update update button icon
- [ ] Add RepoTreeWidget.cpp to CMakeLists SOURCES
- [ ] **REVIEW RepoTreeWidget.h/.cpp**
- [ ] Fix any issues from review

### 3.6 ChangesTreeWidget
- [ ] Create `src/widgets/ChangesTreeWidget.h`
- [ ] Define ChangesTreeWidget class (inherits QTreeWidget)
  - [ ] Q_OBJECT
  - [ ] Constructor
  - [ ] Signal: filesChecked(QStringList files)
  - [ ] Signal: diffRequested(QString file)
  - [ ] Slot: setChanges(QList<FileChange> unstaged, QList<FileChange> staged)
  - [ ] Slot: clearChanges()
  - [ ] getCheckedFiles() -> QStringList
  - [ ] Private: QTreeWidgetItem* m_allItem
  - [ ] Private: QList<QTreeWidgetItem*> m_fileItems
  - [ ] Private slot: onItemChanged(QTreeWidgetItem*, int)
  - [ ] Private slot: onContextMenu(QPoint)
- [ ] Create `src/widgets/ChangesTreeWidget.cpp`
- [ ] Implement constructor
  - [ ] Set checkable
  - [ ] Set context menu policy
  - [ ] Connect itemChanged signal
  - [ ] Connect customContextMenuRequested
  - [ ] Apply dark theme styling
- [ ] Implement setChanges()
  - [ ] Clear tree
  - [ ] Add "---- Modified files ----" header
  - [ ] Add "-- All --" checkable item
  - [ ] Add each unstaged file with checkbox
  - [ ] Set tooltip with file path
  - [ ] Add "---- Cached files ----" header if staged files exist
  - [ ] Add each staged file (no checkbox)
- [ ] Implement onItemChanged()
  - [ ] If "All" toggled, toggle all file items
  - [ ] Emit filesChecked with checked files
- [ ] Implement onContextMenu()
  - [ ] Create menu with "Check diff"
  - [ ] If action selected, emit diffRequested
- [ ] Implement getCheckedFiles()
- [ ] Add ChangesTreeWidget.cpp to CMakeLists SOURCES
- [ ] **REVIEW ChangesTreeWidget.h/.cpp**
- [ ] Fix any issues from review

---

## Phase 4: Integration

### 4.1 MainScreen Layout
- [ ] Create `src/screens/MainScreen.h`
- [ ] Define MainScreen class (inherits QWidget)
  - [ ] Q_OBJECT
  - [ ] Constructor
  - [ ] Signal: statusChanged(QString, QString)
  - [ ] Signal: gitTaskRequested(GitTaskRequest)
  - [ ] Slot: onGitTaskCompleted(GitTaskResult)
  - [ ] Slot: onRepoSelected(QString path)
  - [ ] Slot: onBranchChanged(QString branch)
  - [ ] Slot: onFilesChecked(QStringList files)
  - [ ] Private: RepoTreeWidget* m_repoTree
  - [ ] Private: ChangesTreeWidget* m_changesTree
  - [ ] Private: BranchSelector* m_branchSelector
  - [ ] Private: QComboBox* m_mergeSelector
  - [ ] Private: StatusBar* m_statusBar
  - [ ] Private: QLineEdit* m_messageInput
  - [ ] Private: All button pointers (pull, push, commit, etc.)
  - [ ] Private: QString m_currentRepoPath
  - [ ] Private: bool m_isExtended
  - [ ] Private: void setupUi()
  - [ ] Private: void setupConnections()
  - [ ] Private: void applyStyle()
- [ ] Create `src/screens/MainScreen.cpp`
- [ ] Implement setupUi()
  - [ ] Create main horizontal layout
  - [ ] Left panel: folder_tree (RepoTreeWidget)
  - [ ] Right panel: vertical layout
    - [ ] Repo label
    - [ ] Branch selector + merge selector row
    - [ ] Changes tree
    - [ ] Message input
    - [ ] Button rows (per VIEWS.md layout)
    - [ ] Status bar
  - [ ] Set sizes per VIEWS.md coordinates
- [ ] Implement setupConnections()
  - [ ] Connect RepoTreeWidget::repoSelected -> onRepoSelected
  - [ ] Connect BranchSelector::branchChanged -> onBranchChanged
  - [ ] Connect ChangesTreeWidget::filesChecked -> onFilesChecked
  - [ ] Connect all buttons to their handlers
  - [ ] Connect Ctrl key press/release for destructive button unlock
- [ ] Implement applyStyle()
  - [ ] Apply dark theme palette
  - [ ] Set colors per VIEWS.md section 3
- [ ] Implement button click handlers
  - [ ] onPullClicked()
  - [ ] onPushClicked()
  - [ ] onCommitClicked()
  - [ ] onCommitPushClicked()
  - [ ] onIgnoreClicked()
  - [ ] onMergeClicked()
  - [ ] onUpdateClicked()
  - [ ] onDeleteBranchClicked()
  - [ ] onResetClicked()
  - [ ] onRestoreClicked()
  - [ ] onDeleteFileClicked()
  - [ ] onExtendClicked()
- [ ] Implement onRepoSelected()
  - [ ] Update m_currentRepoPath
  - [ ] Request branch list
  - [ ] Request changes list
  - [ ] Enable buttons
- [ ] Implement onBranchChanged()
  - [ ] Queue checkout task
- [ ] Implement onGitTaskCompleted()
  - [ ] Switch on task type
  - [ ] Update UI with results
  - [ ] Show status message
- [ ] Add MainScreen.cpp to CMakeLists SOURCES
- [ ] **REVIEW MainScreen.h/.cpp**
- [ ] Fix any issues from review

### 4.2 AppController
- [ ] Create `src/controller/AppController.h`
- [ ] Define AppController class (inherits QObject)
  - [ ] Q_OBJECT
  - [ ] Constructor
  - [ ] Destructor
  - [ ] void run()
  - [ ] Private: MainScreen* m_mainScreen
  - [ ] Private: GitWorker* m_gitWorker
  - [ ] Private: Config* m_config
  - [ ] Private: FolderTreeModel* m_folderModel
  - [ ] Private: QTimer* m_autoUpdateTimer
  - [ ] Private: void initialize()
  - [ ] Private: void setupConnections()
  - [ ] Private slot: onAutoUpdate()
- [ ] Create `src/controller/AppController.cpp`
- [ ] Implement constructor
  - [ ] Create Config instance
  - [ ] Load config
  - [ ] If user == "user", show warning popup
  - [ ] Validate paths, show warning for invalid
  - [ ] Create GitWorker
  - [ ] Start GitWorker thread
  - [ ] Create FolderTreeModel
  - [ ] Scan configured paths
  - [ ] Create MainScreen
  - [ ] Set model on MainScreen
  - [ ] Setup auto-update timer (30 min)
- [ ] Implement destructor
  - [ ] Stop auto-update timer
  - [ ] Stop GitWorker
  - [ ] Delete components
- [ ] Implement setupConnections()
  - [ ] Connect MainScreen::gitTaskRequested -> GitWorker::queueTask
  - [ ] Connect GitWorker::taskCompleted -> MainScreen::onGitTaskCompleted
  - [ ] Connect GitWorker::progressUpdate -> StatusBar
- [ ] Implement run()
  - [ ] Show MainScreen
  - [ ] Trigger initial status update
- [ ] Add AppController.cpp to CMakeLists SOURCES
- [ ] **REVIEW AppController.h/.cpp**
- [ ] Fix any issues from review

### 4.3 main.cpp
- [ ] Create `src/main.cpp`
- [ ] Include necessary headers
- [ ] Create QApplication
- [ ] Set application style to "Fusion"
- [ ] Create and apply dark theme QPalette
  - [ ] Window: (53, 53, 53)
  - [ ] WindowText: white
  - [ ] Base: (25, 25, 25)
  - [ ] AlternateBase: (53, 53, 53)
  - [ ] ToolTipBase: black
  - [ ] ToolTipText: white
  - [ ] Text: white
  - [ ] Button: (53, 53, 53)
  - [ ] ButtonText: white
  - [ ] BrightText: red
  - [ ] Link: (42, 130, 218)
  - [ ] Highlight: (42, 130, 218)
  - [ ] HighlightedText: black
- [ ] Set window title "GitSardine"
- [ ] Set window icon (when available)
- [ ] Create AppController
- [ ] Call controller.run()
- [ ] Return app.exec()
- [ ] Add main.cpp to CMakeLists SOURCES
- [ ] **REVIEW main.cpp**
- [ ] Fix any issues from review

---

## Phase 5: Final Integration & Testing

### 5.1 CMakeLists Verification
- [ ] Verify all source files listed in SOURCES
- [ ] Verify all include directories correct
- [ ] Verify all link libraries correct
- [ ] Run cmake configuration
- [ ] Fix any CMake errors

### 5.2 Build Test
- [ ] Run full build: `cmake --build .`
- [ ] Fix all compilation errors
- [ ] Fix all linker errors
- [ ] Fix all warnings (treat as errors)
- [ ] Successful clean build

### 5.3 Functional Testing
- [ ] Application launches without crash
- [ ] Config loads correctly (or creates default)
- [ ] User warning popup shows if user == "user"
- [ ] Invalid path warning shows for bad paths
- [ ] Repositories discovered and shown in tree
- [ ] Double-click repo selects it
- [ ] Double-click folder expands/collapses
- [ ] Branch list populates correctly
- [ ] Changes list shows modified files
- [ ] Status icons update correctly
- [ ] Pull button works
- [ ] Push button works
- [ ] Commit button works (with files selected and message)
- [ ] Branch switch works (with stash/pop)
- [ ] Branch create works (validates name)
- [ ] Branch delete works (requires Ctrl, not master)
- [ ] Reset button works
- [ ] Restore button works (requires Ctrl)
- [ ] Delete file works (requires Ctrl)
- [ ] Merge button works
- [ ] Diff viewer opens for text files
- [ ] Extend/collapse button works
- [ ] Auto-update runs every 30 minutes
- [ ] Progress bar shows during operations
- [ ] Error messages display correctly
- [ ] Tooltips show command output

### 5.4 Final Checklist
- [ ] All phases complete
- [ ] All reviews passed
- [ ] No pending issues
- [ ] Clean build
- [ ] All tests pass
- [ ] Application fully functional

---

## Notes

- **REVIEW** items require REVIEWER agent to check work
- If REVIEWER finds issues, responsible agent must fix them
- Review cycle repeats until APPROVED
- No phase is complete until all its items are checked
