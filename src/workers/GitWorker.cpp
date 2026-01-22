#include "GitWorker.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDebug>

GitWorker::GitWorker(QObject *parent)
    : QThread(parent)
    , m_running(true)
{
    // Register metatypes for signal/slot connections
    qRegisterMetaType<GitTaskRequest>("GitTaskRequest");
    qRegisterMetaType<GitTaskResult>("GitTaskResult");
}

GitWorker::~GitWorker()
{
    stopWorker();
    wait();
}

void GitWorker::stopWorker()
{
    QMutexLocker locker(&m_queueMutex);
    m_running = false;
    m_queueCondition.wakeAll();
}

void GitWorker::queueTask(GitTaskRequest request)
{
    QMutexLocker locker(&m_queueMutex);
    m_taskQueue.enqueue(request);
    m_queueCondition.wakeOne();
}

void GitWorker::cancelTask(int requestId)
{
    QMutexLocker locker(&m_queueMutex);
    QQueue<GitTaskRequest> newQueue;
    while (!m_taskQueue.isEmpty()) {
        GitTaskRequest req = m_taskQueue.dequeue();
        if (req.requestId != requestId) {
            newQueue.enqueue(req);
        }
    }
    m_taskQueue = newQueue;
}

void GitWorker::run()
{
    while (m_running) {
        GitTaskRequest request;

        {
            QMutexLocker locker(&m_queueMutex);
            while (m_taskQueue.isEmpty() && m_running) {
                m_queueCondition.wait(&m_queueMutex);
            }

            if (!m_running) {
                break;
            }

            request = m_taskQueue.dequeue();
        }

        GitTaskResult result;
        result.requestId = request.requestId;

        switch (request.task) {
            case GitTask::CheckStatus:
                result = handleCheckStatus(request);
                break;
            case GitTask::CheckAllStatus:
                result = handleCheckAllStatus(request);
                break;
            case GitTask::Fetch:
                result = handleFetch(request);
                break;
            case GitTask::Pull:
                result = handlePull(request);
                break;
            case GitTask::Push:
                result = handlePush(request);
                break;
            case GitTask::Commit:
                result = handleCommit(request);
                break;
            case GitTask::Checkout:
                result = handleCheckout(request);
                break;
            case GitTask::CreateBranch:
                result = handleCreateBranch(request);
                break;
            case GitTask::DeleteBranch:
                result = handleDeleteBranch(request);
                break;
            case GitTask::Merge:
                result = handleMerge(request);
                break;
            case GitTask::Reset:
                result = handleReset(request);
                break;
            case GitTask::Restore:
                result = handleRestore(request);
                break;
            case GitTask::Stash:
                result = handleStash(request);
                break;
            case GitTask::StashPop:
                result = handleStashPop(request);
                break;
            case GitTask::GetBranches:
                result = handleGetBranches(request);
                break;
            case GitTask::GetChanges:
                result = handleGetChanges(request);
                break;
            case GitTask::GetDiff:
                result = handleGetDiff(request);
                break;
        }

        result.task = request.task;
        emit taskCompleted(result);
    }
}

GitWorker::GitCommandResult GitWorker::runGitCommand(const QString& workDir, const QStringList& args)
{
    GitCommandResult result;
    QProcess process;
    process.setWorkingDirectory(workDir);
    process.start("git", args);

    if (!process.waitForFinished(60000)) {  // 60 second timeout
        result.exitCode = -1;
        result.stdErr = "Command timed out";
        return result;
    }

    result.exitCode = process.exitCode();
    result.stdOut = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    result.stdErr = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return result;
}

QString GitWorker::getCurrentBranch(const QString& repoPath)
{
    auto result = runGitCommand(repoPath, {"rev-parse", "--abbrev-ref", "HEAD"});
    if (result.exitCode == 0) {
        return result.stdOut;
    }
    return QString();
}

int GitWorker::getStashCount(const QString& repoPath)
{
    auto result = runGitCommand(repoPath, {"stash", "list"});
    if (result.exitCode == 0 && !result.stdOut.isEmpty()) {
        return result.stdOut.split('\n').count();
    }
    return 0;
}

bool GitWorker::hasUncommittedChanges(const QString& repoPath)
{
    auto result = runGitCommand(repoPath, {"status", "--porcelain"});
    return !result.stdOut.isEmpty();
}

GitTaskResult GitWorker::handleCheckStatus(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    QVariantMap statusData;

    // Get current branch
    QString branch = getCurrentBranch(req.repoPath);
    statusData["currentBranch"] = branch;

    // Check for uncommitted changes
    bool hasChanges = hasUncommittedChanges(req.repoPath);
    statusData["needsCommit"] = hasChanges;

    // Check ahead/behind using rev-list
    auto aheadResult = runGitCommand(req.repoPath, {"rev-list", "--count", "@{u}..HEAD"});
    auto behindResult = runGitCommand(req.repoPath, {"rev-list", "--count", "HEAD..@{u}"});

    int ahead = 0, behind = 0;
    if (aheadResult.exitCode == 0) {
        ahead = aheadResult.stdOut.toInt();
    }
    if (behindResult.exitCode == 0) {
        behind = behindResult.stdOut.toInt();
    }

    statusData["ahead"] = ahead;
    statusData["behind"] = behind;
    statusData["needsPush"] = ahead > 0;
    statusData["needsPull"] = behind > 0;
    statusData["hasError"] = false;

    result.success = true;
    result.data = statusData;
    return result;
}

GitTaskResult GitWorker::handleCheckAllStatus(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    QVariantList allStatus;
    int total = req.args.size();
    int current = 0;

    for (const QString& path : req.args) {
        GitTaskRequest singleReq;
        singleReq.repoPath = path;
        singleReq.requestId = req.requestId;

        GitTaskResult singleResult = handleCheckStatus(singleReq);

        QVariantMap repoStatus;
        repoStatus["path"] = path;
        repoStatus["status"] = singleResult.data;
        repoStatus["success"] = singleResult.success;
        allStatus.append(repoStatus);

        current++;
        int percent = (current * 100) / total;
        emit progressUpdate(req.requestId, percent, QString("Checking %1/%2").arg(current).arg(total));
    }

    result.success = true;
    result.data = allStatus;
    return result;
}

GitTaskResult GitWorker::handleFetch(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    auto cmdResult = runGitCommand(req.repoPath, {"fetch", "-v"});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Fetch failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Fetch successful";
    return result;
}

GitTaskResult GitWorker::handlePull(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    // Check for uncommitted changes
    if (hasUncommittedChanges(req.repoPath)) {
        result.success = false;
        result.message = "Commit or discard changes before pull";
        return result;
    }

    QString branch = getCurrentBranch(req.repoPath);
    auto cmdResult = runGitCommand(req.repoPath, {"pull", "origin", branch});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Pull failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = QString("%1 is up to date").arg(branch);
    return result;
}

GitTaskResult GitWorker::handlePush(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    QString branch = getCurrentBranch(req.repoPath);
    auto cmdResult = runGitCommand(req.repoPath, {"push", "origin", branch});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Push failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Push successful";
    return result;
}

GitTaskResult GitWorker::handleCommit(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    if (req.args.isEmpty()) {
        result.success = false;
        result.message = "Commit message required";
        return result;
    }

    QString message = req.args[0];
    QStringList files = req.args.mid(1);

    // Add files to staging
    if (files.isEmpty()) {
        auto addResult = runGitCommand(req.repoPath, {"add", "-A"});
        if (addResult.exitCode != 0) {
            result.success = false;
            result.message = "Failed to stage files";
            return result;
        }
    } else {
        for (const QString& file : files) {
            auto addResult = runGitCommand(req.repoPath, {"add", file});
            if (addResult.exitCode != 0) {
                result.success = false;
                result.message = QString("Failed to stage: %1").arg(file);
                return result;
            }
        }
    }

    // Commit
    auto cmdResult = runGitCommand(req.repoPath, {"commit", "-m", message});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Commit failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Commit successful";
    return result;
}

GitTaskResult GitWorker::handleCheckout(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    if (req.args.isEmpty()) {
        result.success = false;
        result.message = "Branch name required";
        return result;
    }

    QString branchName = req.args[0];

    // Stash if there are changes
    int stashCountBefore = getStashCount(req.repoPath);
    bool stashCreated = false;

    if (hasUncommittedChanges(req.repoPath)) {
        runGitCommand(req.repoPath, {"stash"});
        stashCreated = getStashCount(req.repoPath) > stashCountBefore;
    }

    // Checkout
    auto cmdResult = runGitCommand(req.repoPath, {"checkout", branchName});

    if (cmdResult.exitCode != 0) {
        // Try creating and checking out the branch (for remote tracking)
        auto createResult = runGitCommand(req.repoPath, {"checkout", "-b", branchName, QString("origin/%1").arg(branchName)});
        if (createResult.exitCode != 0) {
            result.success = false;
            result.message = "Cannot switch branch";
            if (!cmdResult.stdErr.isEmpty()) {
                result.message = cmdResult.stdErr;
            }
            return result;
        }
    }

    // Pop stash if we created one
    if (stashCreated) {
        auto popResult = runGitCommand(req.repoPath, {"stash", "pop"});
        if (popResult.exitCode != 0) {
            result.message = QString("Switched to '%1' but stash pop failed - resolve manually").arg(branchName);
        }
    }

    result.success = true;
    if (result.message.isEmpty()) {
        result.message = QString("Switched to branch '%1'").arg(branchName);
    }
    return result;
}

GitTaskResult GitWorker::handleCreateBranch(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    if (req.args.isEmpty()) {
        result.success = false;
        result.message = "Branch name required";
        return result;
    }

    QString branchName = req.args[0];

    if (branchName.contains(' ')) {
        result.success = false;
        result.message = "Branch name cannot contain spaces";
        return result;
    }

    auto cmdResult = runGitCommand(req.repoPath, {"branch", branchName});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Cannot create branch";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Branch created";
    return result;
}

GitTaskResult GitWorker::handleDeleteBranch(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    if (req.args.isEmpty()) {
        result.success = false;
        result.message = "Branch name required";
        return result;
    }

    QString branchName = req.args[0];

    if (branchName == "master" || branchName == "main") {
        result.success = false;
        result.message = "Cannot delete master/main";
        return result;
    }

    // Checkout master first if we're on the branch to delete
    QString currentBranch = getCurrentBranch(req.repoPath);
    if (currentBranch == branchName) {
        auto checkoutResult = runGitCommand(req.repoPath, {"checkout", "master"});
        if (checkoutResult.exitCode != 0) {
            runGitCommand(req.repoPath, {"checkout", "main"});
        }
    }

    auto cmdResult = runGitCommand(req.repoPath, {"branch", "-D", branchName});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Cannot delete branch";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = QString("%1 deleted").arg(branchName);
    return result;
}

GitTaskResult GitWorker::handleMerge(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    if (req.args.isEmpty()) {
        result.success = false;
        result.message = "Source branch required";
        return result;
    }

    QString sourceBranch = req.args[0];
    QString currentBranch = getCurrentBranch(req.repoPath);

    auto cmdResult = runGitCommand(req.repoPath, {"merge", sourceBranch});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Merge failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    // Auto-delete source branch if on master/main
    if (currentBranch == "master" || currentBranch == "main") {
        runGitCommand(req.repoPath, {"branch", "-d", sourceBranch});
    }

    result.success = true;
    result.message = "Merge successful";
    return result;
}

GitTaskResult GitWorker::handleReset(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    auto cmdResult = runGitCommand(req.repoPath, {"reset"});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Reset failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Reset successful";
    return result;
}

GitTaskResult GitWorker::handleRestore(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    auto cmdResult = runGitCommand(req.repoPath, {"restore", "."});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Restore failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Restore successful";
    return result;
}

GitTaskResult GitWorker::handleStash(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    int stashCountBefore = getStashCount(req.repoPath);
    runGitCommand(req.repoPath, {"stash"});
    int stashCountAfter = getStashCount(req.repoPath);

    result.success = true;
    result.data = (stashCountAfter > stashCountBefore);
    result.message = (stashCountAfter > stashCountBefore) ? "Stash created" : "Nothing to stash";
    return result;
}

GitTaskResult GitWorker::handleStashPop(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    auto cmdResult = runGitCommand(req.repoPath, {"stash", "pop"});

    if (cmdResult.exitCode != 0) {
        result.success = false;
        result.message = "Stash pop failed";
        if (!cmdResult.stdErr.isEmpty()) {
            result.message = cmdResult.stdErr;
        }
        return result;
    }

    result.success = true;
    result.message = "Stash applied";
    return result;
}

GitTaskResult GitWorker::handleGetBranches(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    QStringList localBranches;
    QStringList remoteBranches;

    // Get local branches
    auto localResult = runGitCommand(req.repoPath, {"branch", "--format=%(refname:short)"});
    if (localResult.exitCode == 0 && !localResult.stdOut.isEmpty()) {
        localBranches = localResult.stdOut.split('\n', Qt::SkipEmptyParts);
    }

    // Get remote branches
    auto remoteResult = runGitCommand(req.repoPath, {"branch", "-r", "--format=%(refname:short)"});
    if (remoteResult.exitCode == 0 && !remoteResult.stdOut.isEmpty()) {
        QStringList rawRemotes = remoteResult.stdOut.split('\n', Qt::SkipEmptyParts);
        for (const QString& remote : rawRemotes) {
            QString branch = remote;
            if (branch.startsWith("origin/")) {
                branch = branch.mid(7);
            }
            if (!branch.contains("HEAD") && !localBranches.contains(branch)) {
                remoteBranches.append(branch);
            }
        }
    }

    QString currentBranch = getCurrentBranch(req.repoPath);

    QVariantMap branchData;
    branchData["local"] = localBranches;
    branchData["remote"] = remoteBranches;
    branchData["current"] = currentBranch;

    result.success = true;
    result.data = branchData;
    return result;
}

GitTaskResult GitWorker::handleGetChanges(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    QStringList modifiedFiles;
    QStringList stagedFiles;

    // Get modified files (unstaged)
    auto modifiedResult = runGitCommand(req.repoPath, {"ls-files", "-m", "-d", "-o", "-z", "--exclude-standard"});
    if (modifiedResult.exitCode == 0 && !modifiedResult.stdOut.isEmpty()) {
        modifiedFiles = modifiedResult.stdOut.split(QChar('\0'), Qt::SkipEmptyParts);
    }

    // Get staged files
    auto stagedResult = runGitCommand(req.repoPath, {"diff", "--name-only", "--cached"});
    if (stagedResult.exitCode == 0 && !stagedResult.stdOut.isEmpty()) {
        stagedFiles = stagedResult.stdOut.split('\n', Qt::SkipEmptyParts);
    }

    // Filter out built-in ignore patterns
    QStringList ignorePatterns = {".idea/", "__pycache__/", "venv/"};
    QStringList filteredModified;
    for (const QString& file : modifiedFiles) {
        bool ignore = false;
        for (const QString& pattern : ignorePatterns) {
            if (file.startsWith(pattern) || file.contains("/" + pattern)) {
                ignore = true;
                break;
            }
        }
        if (!ignore && !stagedFiles.contains(file)) {
            filteredModified.append(file);
        }
    }

    filteredModified.sort(Qt::CaseInsensitive);
    stagedFiles.sort(Qt::CaseInsensitive);

    QVariantMap changeData;
    changeData["modified"] = filteredModified;
    changeData["staged"] = stagedFiles;

    result.success = true;
    result.data = changeData;
    return result;
}

GitTaskResult GitWorker::handleGetDiff(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    if (req.args.isEmpty()) {
        result.success = false;
        result.message = "File path required";
        return result;
    }

    QString filePath = req.args[0];

    // Check for binary extensions
    QStringList binaryExtensions = {
        ".ods", ".odg", ".odt", ".Z3PRT", ".Z3ASM", ".exe",
        ".Z3DRW", ".stp", ".step", ".xrs", ".pdf"
    };

    for (const QString& ext : binaryExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            result.success = false;
            result.message = "Binary file - diff not available";
            return result;
        }
    }

    auto cmdResult = runGitCommand(req.repoPath, {"diff", filePath});

    result.success = true;
    result.data = cmdResult.stdOut;
    return result;
}
