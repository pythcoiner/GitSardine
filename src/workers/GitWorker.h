#ifndef GITWORKER_H
#define GITWORKER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

#ifdef USE_LIBGIT2
#include <git2.h>
#endif

/**
 * Git task types that can be executed by GitWorker
 */
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

/**
 * Request structure for git tasks
 */
struct GitTaskRequest {
    GitTask task;
    QString repoPath;
    QStringList args;       // task-specific arguments
    int requestId;          // for matching responses

    GitTaskRequest()
        : task(GitTask::CheckStatus)
        , requestId(0)
    {}
};

/**
 * Result structure for git tasks
 */
struct GitTaskResult {
    int requestId;
    GitTask task;           // which task this result is for
    bool success;
    QString message;
    QVariant data;          // task-specific result data

    GitTaskResult()
        : requestId(0)
        , task(GitTask::CheckStatus)
        , success(false)
    {}
};

/**
 * GitWorker - Single thread for ALL git operations
 *
 * Uses libgit2 if available, falls back to QProcess/git CLI otherwise.
 */
class GitWorker : public QThread {
    Q_OBJECT

public:
    explicit GitWorker(QObject *parent = nullptr);
    ~GitWorker() override;

signals:
    void taskCompleted(GitTaskResult result);
    void progressUpdate(int requestId, int percent, QString status);

public slots:
    void queueTask(GitTaskRequest request);
    void cancelTask(int requestId);
    void stopWorker();

protected:
    void run() override;

private:
    QQueue<GitTaskRequest> m_taskQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    bool m_running;

    // Helper to run git command via QProcess
    struct GitCommandResult {
        int exitCode;
        QString stdOut;
        QString stdErr;
    };
    GitCommandResult runGitCommand(const QString& workDir, const QStringList& args);

    // Task handlers (QProcess-based)
    GitTaskResult handleCheckStatus(const GitTaskRequest& req);
    GitTaskResult handleCheckAllStatus(const GitTaskRequest& req);
    GitTaskResult handleFetch(const GitTaskRequest& req);
    GitTaskResult handlePull(const GitTaskRequest& req);
    GitTaskResult handlePush(const GitTaskRequest& req);
    GitTaskResult handleCommit(const GitTaskRequest& req);
    GitTaskResult handleCheckout(const GitTaskRequest& req);
    GitTaskResult handleCreateBranch(const GitTaskRequest& req);
    GitTaskResult handleDeleteBranch(const GitTaskRequest& req);
    GitTaskResult handleMerge(const GitTaskRequest& req);
    GitTaskResult handleReset(const GitTaskRequest& req);
    GitTaskResult handleRestore(const GitTaskRequest& req);
    GitTaskResult handleStash(const GitTaskRequest& req);
    GitTaskResult handleStashPop(const GitTaskRequest& req);
    GitTaskResult handleGetBranches(const GitTaskRequest& req);
    GitTaskResult handleGetChanges(const GitTaskRequest& req);
    GitTaskResult handleGetDiff(const GitTaskRequest& req);

    // Helper functions
    QString getCurrentBranch(const QString& repoPath);
    int getStashCount(const QString& repoPath);
    bool hasUncommittedChanges(const QString& repoPath);
};

// Register metatypes for signal/slot
Q_DECLARE_METATYPE(GitTaskRequest)
Q_DECLARE_METATYPE(GitTaskResult)

#endif // GITWORKER_H
