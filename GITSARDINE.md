# GitSardine C++ Port - Implementation Plan

## Related Specification Documents

```
+----------------------------------------------------------+
| Document        | Contents                               |
+-----------------+----------------------------------------+
| VIEWS.md        | Pixel-perfect UI layout, coordinates,  |
|                 | colors, widget specs, icons            |
+-----------------+----------------------------------------+
| FEATURES.md     | Git operations, status checking,       |
|                 | file operations, threading model       |
+-----------------+----------------------------------------+
| ACTIONS.md      | User interactions, button handlers,    |
|                 | signal/slot connections, exec chains   |
+-----------------+----------------------------------------+
| DATA_MODELS.md  | Config JSON schema, constants,         |
|                 | messages, icon filenames               |
+-----------------+----------------------------------------+
```

---

## 1. Project Structure

```
gitsardine-cpp/
+-- CMakeLists.txt
+-- src/
|   +-- main.cpp
|   +-- core/
|   |   +-- Result.h               (Result<T,E> error handling)
|   +-- controller/
|   |   +-- AppController.h/.cpp
|   +-- screens/
|   |   +-- MainScreen.h/.cpp
|   +-- widgets/
|   |   +-- RepoTreeWidget.h/.cpp
|   |   +-- ChangesTreeWidget.h/.cpp
|   |   +-- BranchSelector.h/.cpp
|   |   +-- StatusBar.h/.cpp
|   +-- git/
|   |   +-- GitRepository.h/.cpp
|   |   +-- GitManager.h/.cpp
|   |   +-- GitStatus.h/.cpp
|   +-- config/
|   |   +-- Config.h/.cpp
|   +-- models/
|   |   +-- RepoModel.h/.cpp
|   |   +-- FolderTreeModel.h/.cpp
|   +-- workers/
|       +-- StatusUpdateWorker.h/.cpp
|       +-- GitOperationWorker.h/.cpp
+-- resources/
|   +-- icons/
|       +-- icons.h          (includes all icon headers)
|       +-- *.h              (embedded PNG byte arrays)
+-- deps/
    +-- qontrol/             (git submodule)
```

---

## 2. Dependencies

```
+--------------+------------------------------+------------------------------+
| Dependency   | Purpose                      | Integration                  |
+--------------+------------------------------+------------------------------+
| Qt6 (static) | GUI framework                | via qt_static                |
| qontrol      | Qt application framework     | git submodule                |
| libgit2      | Git operations               | system package or bundled    |
+--------------+------------------------------+------------------------------+
```

**Note:** Configuration uses Qt's built-in QJsonDocument (no external dependencies).

### 2.1 Embedded Icons (no .qrc)

Icons embedded as raw byte arrays in `.h` files (same pattern as qontrol):

```cpp
// resources/icons/download.h
unsigned char download_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
    // ... more bytes
};
unsigned int download_png_len = 482;
```

Usage:
```cpp
#include "resources/icons/download.h"

QPixmap pixmap;
pixmap.loadFromData(download_png, download_png_len, "PNG");
QIcon icon(pixmap);
```

Generate from PNG:
```bash
xxd -i icon.png > icon.h
```

### 2.2 Target Platforms

- **Linux** (native build)
- **Windows** (cross-compiled from Linux via qt_static)
- **macOS ARM64** (cross-compiled, requires Xcode SDK)
- **macOS x86_64** (cross-compiled, requires Xcode SDK)

### 2.3 Error Handling (Result Type)

**No exceptions.** All error handling uses a Rust-like Result pattern:

```cpp
// src/core/Result.h
template<typename T, typename E = QString>
class Result {
public:
    static Result Ok(T value);
    static Result Err(E error);

    bool isOk() const;
    bool isErr() const;
    T value() const;      // Call only if isOk()
    E error() const;      // Call only if isErr()

    // Optional: monadic operations
    template<typename F>
    auto map(F&& f) -> Result<decltype(f(value())), E>;

    template<typename F>
    auto mapErr(F&& f) -> Result<T, decltype(f(error()))>;
};

// Convenience type aliases
using VoidResult = Result<std::monostate, QString>;
```

**Usage pattern:**

```cpp
Result<QString, QString> GitManager::createBranch(const QString& name) {
    if (name.isEmpty()) {
        return Result<QString, QString>::Err("Branch name required");
    }
    if (branchExists(name)) {
        return Result<QString, QString>::Err("Branch already exists");
    }

    // libgit2 operation...
    int error = git_branch_create(&ref, repo, name.toUtf8(), commit, 0);
    if (error < 0) {
        return Result<QString, QString>::Err(git_error_last()->message);
    }

    return Result<QString, QString>::Ok("Branch created");
}

// Caller handles result
auto result = gitManager->createBranch(branchName);
if (result.isErr()) {
    setLabel(result.error());
} else {
    setLabel(result.value());
    updateBranch();
}
```

**Rules:**
- Functions that can fail return `Result<T, E>`
- Never throw exceptions
- Never use `assert()` for recoverable errors
- Error messages are user-friendly (see DATA_MODELS.md section 4.2)

---

## 3. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.17)
project(gitsardine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Static Qt6 from qt_static (adjust path per platform)
# Linux: qt_static/dist/linux
# Windows: qt_static/dist/windows
# macOS ARM: qt_static/dist/macos-arm
# macOS x86: qt_static/dist/macos-x86
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/qt_static/dist/linux")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

# Add qontrol
add_subdirectory(deps/qontrol)

# Sources
set(SOURCES
    src/main.cpp
    src/controller/AppController.cpp
    src/screens/MainScreen.cpp
    src/widgets/RepoTreeWidget.cpp
    src/widgets/ChangesTreeWidget.cpp
    src/widgets/BranchSelector.cpp
    src/widgets/StatusBar.cpp
    src/widgets/DiffViewerDialog.cpp
    src/git/GitRepository.cpp
    src/git/GitManager.cpp
    src/git/GitStatus.cpp
    src/config/Config.cpp
    src/models/RepoModel.cpp
    src/models/FolderTreeModel.cpp
    src/workers/GitWorker.cpp
)

add_executable(gitsardine ${SOURCES})

target_include_directories(gitsardine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${LIBGIT2_INCLUDE_DIRS}
)

target_link_libraries(gitsardine PRIVATE
    qontrol
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${LIBGIT2_LIBRARIES}
)
```

---

## 4. Implementation Phases

### Phase 1: Project Setup & Infrastructure

1. Create project structure with directories
2. Set up CMake with qt_static, qontrol, libgit2
3. Create basic qontrol application with empty MainScreen
4. Verify build on Linux

### Phase 2: Configuration System

1. Implement Config class - parse/write JSON (see DATA_MODELS.md for schema)
2. Handle defaults - create config.json if missing
3. Support all fields: paths, user, extend, ignore

### Phase 3: Git Layer (libgit2)

1. **GitRepository class** - wrapper around git_repository
   - Open repository
   - Get current branch
   - Get file status (modified, staged, untracked)
   - Get ahead/behind counts
2. **GitManager class** - high-level operations (see FEATURES.md)
   - Pull (fetch + merge)
   - Push
   - Commit (stage files + commit)
   - Create/delete/switch branch
   - Reset, restore
3. **GitStatus class** - status data structures

### Phase 4: Repository Discovery

1. Implement path scanning - recursive directory walk (see FEATURES.md 1.1)
2. Build folder tree model - hierarchical structure
3. Create RepoModel - store repository metadata

### Phase 5: Main UI (qontrol)

1. MainScreen layout using qontrol Row/Column (see VIEWS.md for coordinates)
2. Repository tree widget - QTreeView with custom model
3. Changes tree widget - checkable file list
4. Branch selector - QComboBox
5. Status bar - label + progress bar
6. Action buttons - all git operation buttons

### Phase 6: Background Workers

1. **GitWorker** - Single QThread for ALL git/libgit2 operations
   - Receives tasks via signal (task type + parameters)
   - Sends results via signal (result data + status)
   - Owns all `git_repository*` handles (thread safety)
   - See section 5.5 for architecture details
2. Signal/slot connections for progress updates (see ACTIONS.md 5)

### Phase 7: Features & Polish

1. Ctrl-key protection for destructive operations (see ACTIONS.md 3)
2. Dark theme styling (see VIEWS.md 2)
3. Icon resources and status indicators (see DATA_MODELS.md 3)
4. Error handling and user feedback (see DATA_MODELS.md 4)
5. Diff viewer for text files

### Phase 8: Testing & Cross-Platform

1. Test all git operations
2. Windows build via qt_static cross-compilation
3. macOS ARM build via qt_static
4. macOS x86 build via qt_static
5. Create build scripts for each platform target

---

## 5. libgit2 Examples

### Opening a Repository

```cpp
git_repository *repo = nullptr;
int error = git_repository_open(&repo, path.toUtf8().constData());
if (error < 0) {
    // Handle error
}
```

### Getting Status

```cpp
git_status_options opts = GIT_STATUS_OPTIONS_INIT;
opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;

git_status_foreach_ext(repo, &opts, [](const char *path, unsigned int flags, void *payload) {
    auto *list = static_cast<QStringList*>(payload);
    if (flags & GIT_STATUS_WT_MODIFIED) { /* modified */ }
    if (flags & GIT_STATUS_WT_NEW) { /* untracked */ }
    if (flags & GIT_STATUS_INDEX_NEW) { /* staged new */ }
    if (flags & GIT_STATUS_INDEX_MODIFIED) { /* staged modified */ }
    return 0;
}, &fileList);
```

### Fetch Operation

```cpp
git_remote *remote = nullptr;
git_remote_lookup(&remote, repo, "origin");

git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
git_remote_fetch(remote, nullptr, &opts, "fetch");

git_remote_free(remote);
```

### Authentication (SSH/HTTPS)

```
+----------------------------------------------------------+
| CREDENTIAL HANDLING: Rely on System SSH Agent            |
+----------------------------------------------------------+
| The application does NOT implement credential callbacks. |
| Users must configure SSH authentication externally:      |
|                                                          |
| Prerequisites:                                           |
| - SSH key generated (~/.ssh/id_rsa or id_ed25519)        |
| - SSH key added to remote (GitHub, GitLab, etc.)         |
| - SSH agent running with key loaded (ssh-add)            |
| - Remote URL uses SSH format (git@github.com:...)        |
|                                                          |
| For HTTPS remotes:                                       |
| - Use git credential helper (configured in ~/.gitconfig) |
| - Or convert remote URL to SSH format                    |
|                                                          |
| On auth failure:                                         |
| - libgit2 returns error code                             |
| - Show error: "Authentication failed - check SSH setup"  |
| - Do NOT prompt for credentials in the app               |
+----------------------------------------------------------+
```

### Ahead/Behind

```cpp
git_reference *local = nullptr, *upstream = nullptr;
git_reference_lookup(&local, repo, "refs/heads/main");
git_branch_upstream(&upstream, local);

size_t ahead, behind;
git_graph_ahead_behind(&ahead, &behind, repo,
    git_reference_target(local),
    git_reference_target(upstream));
```

### 5.5 GitWorker Thread Architecture (Thread Safety)

**Problem:** libgit2 is not thread-safe. Multiple threads accessing the same
`git_repository*` handle causes race conditions and crashes.

**Solution:** Single `GitWorker` QThread owns all libgit2 operations.

```
+------------------------------------------------------------------+
| ARCHITECTURE: Single Git Worker Thread                           |
+------------------------------------------------------------------+
|                                                                  |
|  Main Thread (UI)                    GitWorker Thread            |
|  +--------------+                    +--------------------+      |
|  |              |  taskRequested     |                    |      |
|  | MainScreen   | -----------------> | GitWorker          |      |
|  |              |    (signal)        |                    |      |
|  |              |                    | - git_repository*  |      |
|  |              |  taskCompleted     | - executes libgit2 |      |
|  |              | <----------------- | - queues tasks     |      |
|  |              |    (signal)        |                    |      |
|  +--------------+                    +--------------------+      |
|                                                                  |
+------------------------------------------------------------------+
```

**GitWorker Interface:**

```cpp
// src/workers/GitWorker.h

enum class GitTask {
    CheckStatus,        // Check single repo status
    CheckAllStatus,     // Check all repos status
    Fetch,              // git fetch
    Pull,               // git pull
    Push,               // git push
    Commit,             // git add + commit
    Checkout,           // git checkout (with stash/pop)
    CreateBranch,       // git branch
    DeleteBranch,       // git branch -D
    Merge,              // git merge
    Reset,              // git reset
    Restore,            // git restore
    Stash,              // git stash
    StashPop,           // git stash pop
    GetBranches,        // list branches
    GetChanges,         // list modified files
    GetDiff             // git diff for single file
};

struct GitTaskRequest {
    GitTask task;
    QString repoPath;
    QStringList args;       // task-specific arguments
    int requestId;          // for matching responses
};

struct GitTaskResult {
    int requestId;
    bool success;
    QString message;
    QVariant data;          // task-specific result data
};

class GitWorker : public QThread {
    Q_OBJECT
public:
    explicit GitWorker(QObject *parent = nullptr);
    ~GitWorker();

signals:
    void taskCompleted(GitTaskResult result);
    void progressUpdate(int requestId, int percent, QString status);

public slots:
    void queueTask(GitTaskRequest request);
    void cancelTask(int requestId);

protected:
    void run() override;

private:
    QQueue<GitTaskRequest> m_taskQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    bool m_running = true;

    // libgit2 handles (owned by this thread only)
    QHash<QString, git_repository*> m_openRepos;

    git_repository* getRepo(const QString& path);
    void closeAllRepos();

    // Task handlers
    GitTaskResult handleCheckStatus(const GitTaskRequest& req);
    GitTaskResult handlePull(const GitTaskRequest& req);
    GitTaskResult handlePush(const GitTaskRequest& req);
    // ... etc
};
```

**Usage from Main Thread:**

```cpp
// In MainScreen or AppController
connect(m_gitWorker, &GitWorker::taskCompleted,
        this, &MainScreen::onGitTaskCompleted);

// Request a pull operation
GitTaskRequest req;
req.task = GitTask::Pull;
req.repoPath = currentRepoPath;
req.args = {branchName};
req.requestId = generateRequestId();

m_gitWorker->queueTask(req);
showProgressBar();

// Handle result
void MainScreen::onGitTaskCompleted(GitTaskResult result) {
    hideProgressBar();
    if (result.success) {
        setLabel("Pull successful");
    } else {
        setLabel("Pull failed", result.message);
    }
}
```

**Thread Safety Rules:**
1. All libgit2 calls happen ONLY in GitWorker thread
2. Main thread sends requests via signal, never calls libgit2 directly
3. GitWorker owns and manages all `git_repository*` handles
4. Results are returned via signal with copied data (no shared pointers)
5. `git_libgit2_init()` called once at GitWorker construction
6. `git_libgit2_shutdown()` called at GitWorker destruction

---

## 6. Files to Create (Ordered)

```
+-----+-------------------------------------------+-----------------------------------+
| #   | File                                      | Purpose                           |
+-----+-------------------------------------------+-----------------------------------+
| 1   | CMakeLists.txt                            | Build configuration               |
| 2   | src/main.cpp                              | Entry point                       |
| 3   | src/core/Result.h                         | Result<T,E> error handling        |
| 4   | src/config/Config.h/.cpp                  | Configuration management          |
| 5   | src/git/GitStatus.h                       | Status data structures            |
| 6   | src/git/GitRepository.h/.cpp              | libgit2 wrapper                   |
| 7   | src/git/GitManager.h/.cpp                 | High-level git operations         |
| 8   | src/models/RepoModel.h/.cpp               | Repository data model             |
| 9   | src/models/FolderTreeModel.h/.cpp         | Tree structure model              |
| 10  | src/workers/GitWorker.h/.cpp              | Single git thread (all libgit2)   |
| 11  | src/widgets/RepoTreeWidget.h/.cpp         | Repository tree view              |
| 12  | src/widgets/ChangesTreeWidget.h/.cpp      | File changes list                 |
| 13  | src/widgets/BranchSelector.h/.cpp         | Branch dropdown                   |
| 14  | src/widgets/StatusBar.h/.cpp              | Status display                    |
| 15  | src/widgets/DiffViewerDialog.h/.cpp       | Diff viewer modal dialog          |
| 16  | src/screens/MainScreen.h/.cpp             | Main UI screen                    |
| 17  | src/controller/AppController.h/.cpp       | Application controller            |
| 18  | resources/icons/icons.h                   | Master include for all icons      |
| 19  | resources/icons/*.h                       | Embedded PNG byte arrays          |
| 20  | resources/gitsardine.desktop                  | Linux desktop entry               |
+-----+-------------------------------------------+-----------------------------------+
```

---

## 7. Verification Plan

1. **Build verification**: `cmake --build .` succeeds
2. **Basic launch**: Application window opens
3. **Config loading**: Reads config.json correctly
4. **Repository discovery**: Finds repos in configured paths
5. **Status display**: Icons update correctly
6. **Git operations**:
   - Create test repo
   - Make changes, verify detection
   - Commit, verify status update
   - Create branch, switch, delete
   - Push/pull (requires remote)
7. **UI responsiveness**: Operations don't freeze UI
8. **Error handling**: Invalid paths, git errors shown properly

---

## 8. Linux Packaging

### 8.1 Desktop Entry

Generate `gitsardine.desktop` at **build time** (not runtime):

```ini
[Desktop Entry]
Name=GitSardine
Comment=Git Repository Manager
Exec=/usr/local/bin/gitsardine
Icon=gitsardine
Terminal=false
Type=Application
Categories=Development;VersionControl;
```

### 8.2 Debian Package

Build `.deb` package for Linux distribution:

```
+----------------------------------------------------------+
| Package Contents                                         |
+----------------------------------------------------------+
| /usr/local/bin/gitsardine          | Main binary             |
| /usr/share/applications/       | gitsardine.desktop          |
|   gitsardine.desktop               |                         |
| /usr/share/icons/hicolor/      | Application icons       |
|   48x48/apps/gitsardine.png        |                         |
| /usr/share/doc/gitsardine/         | Documentation           |
+----------------------------------------------------------+
```

### 8.3 CMake Packaging Support

Add to CMakeLists.txt:

```cmake
# Install targets
install(TARGETS gitsardine RUNTIME DESTINATION bin)
install(FILES resources/gitsardine.desktop
        DESTINATION share/applications)
install(FILES resources/icons/gitsardine_icon.png
        DESTINATION share/icons/hicolor/48x48/apps
        RENAME gitsardine.png)

# CPack configuration for .deb
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "maintainer")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgit2-1.0")
include(CPack)
```

---

## 9. Configuration File Location

Config file uses **platform standard paths** (not same directory as executable):

```
+----------------------------------------------------------+
| Platform        | Config Path                            |
+-----------------+----------------------------------------+
| Linux           | ~/.config/gitsardine/config.json           |
| Windows         | %APPDATA%/GitSardine/config.json           |
| macOS           | ~/Library/Application Support/         |
|                 |   GitSardine/config.json                   |
+-----------------+----------------------------------------+
```

**Behavior:**
- If config directory doesn't exist, create it
- If config.json doesn't exist, create with defaults
- If a configured path is invalid, show warning dialog and skip it (continue with valid paths)

---

## 10. Startup Sequence

Application initialization order:

```
+-----+----------------------------------------------------------+
| #   | Step                                                     |
+-----+----------------------------------------------------------+
| 1   | Initialize Qt application with Fusion style              |
| 2   | Apply dark theme palette                                 |
| 3   | Initialize libgit2 (git_libgit2_init)                    |
| 4   | Load config from platform-standard location              |
|     | - Create default config if missing                       |
| 5   | If user == "user", show warning popup                    |
| 6   | Validate configured paths                                |
|     | - Show dialog for invalid paths, skip them               |
| 7   | Scan valid paths for git repositories                    |
| 8   | Build folder tree model                                  |
| 9   | Setup UI and signal/slot connections                     |
| 10  | Disable all action buttons (no repo selected)            |
| 11  | Lock destructive buttons (Ctrl required)                 |
| 12  | Start background status update for all repos             |
| 13  | Start 30-minute auto-update timer                        |
+-----+----------------------------------------------------------+
```

---

## 11. Shutdown Sequence

Application cleanup on window close:

```
+-----+----------------------------------------------------------+
| #   | Step                                                     |
+-----+----------------------------------------------------------+
| 1   | Signal all running worker threads to stop                |
| 2   | Wait for threads to complete (with timeout)              |
| 3   | Cleanup libgit2 (git_libgit2_shutdown)                   |
| 4   | Close window and exit                                    |
+-----+----------------------------------------------------------+
```

---

## 12. C++ Improvements Over Python

Features improved or added in the C++ port:

```
+----------------------------------------------------------+
| Feature                  | Python           | C++         |
+--------------------------+------------------+-------------+
| Branch switch behavior   | Auto-commits     | Uses git    |
|                          | "change_branch"  | stash       |
+--------------------------+------------------+-------------+
| Changes auto-refresh     | Disabled         | Enabled     |
|                          |                  | (2s timer)  |
+--------------------------+------------------+-------------+
| Diff viewer dialog       | Not implemented  | Full dialog |
|                          | (tooltip only)   | with syntax |
+--------------------------+------------------+-------------+
| Folder tree context menu | Not implemented  | Implemented |
+--------------------------+------------------+-------------+
| Invalid path handling    | Crashes with     | User dialog,|
|                          | exception        | Result type |
+--------------------------+------------------+-------------+
| Config location          | Same dir as exe  | Platform    |
|                          |                  | standard    |
+--------------------------+------------------+-------------+
```
