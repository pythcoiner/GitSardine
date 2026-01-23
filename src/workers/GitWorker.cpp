#include "GitWorker.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <git2.h>

// Certificate check callback - accept known hosts
static int certificate_check_callback(git_cert *cert, int valid, const char *host, void *payload)
{
    Q_UNUSED(cert);
    Q_UNUSED(payload);
    qDebug() << "Certificate check for host:" << host << "valid:" << valid;
    // For SSH, we trust the host (like ssh with StrictHostKeyChecking=no)
    // In production, you'd want to verify against known_hosts
    return 0;  // 0 = accept, negative = reject
}

// SSH credential callback for libgit2
// payload contains a pointer to retry counter
static int credentials_callback(git_credential **out, const char *url,
                                 const char *username_from_url,
                                 unsigned int allowed_types, void *payload)
{
    int* attempts = static_cast<int*>(payload);
    if (attempts) {
        (*attempts)++;
        if (*attempts > 3) {
            qDebug() << "Auth failed after" << (*attempts - 1) << "attempts, giving up";
            return GIT_EUSER;
        }
    }

    qDebug() << "Auth requested for:" << url << "user:" << username_from_url
             << "allowed:" << allowed_types << "attempt:" << (attempts ? *attempts : 0);

    // Try SSH agent first (most common for git over SSH)
    if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
        const char* user = username_from_url ? username_from_url : "git";

        // Check if SSH_AUTH_SOCK is available
        const char* authSock = getenv("SSH_AUTH_SOCK");
        if (authSock && authSock[0] != '\0') {
            int ret = git_credential_ssh_key_from_agent(out, user);
            if (ret == 0) {
                qDebug() << "Using SSH agent for auth";
                return 0;
            }
            qDebug() << "SSH agent failed (ret=" << ret << ")";
        } else {
            qDebug() << "SSH_AUTH_SOCK not set, skipping agent";
        }

        // Fallback to SSH key files - list all keys in ~/.ssh/
        QDir sshDir(QDir::homePath() + "/.ssh");
        QStringList pubKeys = sshDir.entryList(QStringList() << "*.pub", QDir::Files);

        for (const QString& pubFile : pubKeys) {
            QString privKey = sshDir.filePath(pubFile.chopped(4));  // remove .pub
            QString pubKey = sshDir.filePath(pubFile);

            if (QFile::exists(privKey)) {
                qDebug() << "Trying SSH key:" << privKey;
                int ret = git_credential_ssh_key_new(out, user,
                    pubKey.toUtf8().constData(),
                    privKey.toUtf8().constData(),
                    nullptr);  // passphrase - nullptr means no passphrase
                if (ret == 0) {
                    return 0;
                }
                qDebug() << "Key failed (ret=" << ret << ")";
            }
        }
        qDebug() << "No working SSH keys found in ~/.ssh/";
    }

    // Userpass for HTTPS (not implemented yet)
    if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        qDebug() << "HTTPS auth not implemented";
        return GIT_EUSER;
    }

    qDebug() << "No suitable auth method found";
    return GIT_EUSER;
}

// RAII wrapper for git_repository
class GitRepo {
public:
    git_repository* repo = nullptr;

    GitRepo() = default;
    ~GitRepo() { if (repo) git_repository_free(repo); }

    bool open(const QString& path) {
        return git_repository_open(&repo, path.toUtf8().constData()) == 0;
    }

    operator git_repository*() { return repo; }
    git_repository* get() { return repo; }
};

// RAII wrapper for git_reference
class GitRef {
public:
    git_reference* ref = nullptr;

    GitRef() = default;
    ~GitRef() { if (ref) git_reference_free(ref); }

    operator git_reference*() { return ref; }
    git_reference** ptr() { return &ref; }
};

// RAII wrapper for git_remote
class GitRemote {
public:
    git_remote* remote = nullptr;

    GitRemote() = default;
    ~GitRemote() { if (remote) git_remote_free(remote); }

    operator git_remote*() { return remote; }
    git_remote** ptr() { return &remote; }
};

// RAII wrapper for git_index
class GitIndex {
public:
    git_index* index = nullptr;

    GitIndex() = default;
    ~GitIndex() { if (index) git_index_free(index); }

    operator git_index*() { return index; }
    git_index** ptr() { return &index; }
};

// RAII wrapper for git_signature
class GitSignature {
public:
    git_signature* sig = nullptr;

    GitSignature() = default;
    ~GitSignature() { if (sig) git_signature_free(sig); }

    operator git_signature*() { return sig; }
    git_signature** ptr() { return &sig; }
};

// RAII wrapper for git_status_list
class GitStatusList {
public:
    git_status_list* list = nullptr;

    GitStatusList() = default;
    ~GitStatusList() { if (list) git_status_list_free(list); }

    operator git_status_list*() { return list; }
    git_status_list** ptr() { return &list; }
};

// RAII wrapper for git_diff
class GitDiff {
public:
    git_diff* diff = nullptr;

    GitDiff() = default;
    ~GitDiff() { if (diff) git_diff_free(diff); }

    operator git_diff*() { return diff; }
    git_diff** ptr() { return &diff; }
};

// RAII wrapper for git_object
class GitObject {
public:
    git_object* obj = nullptr;

    GitObject() = default;
    ~GitObject() { if (obj) git_object_free(obj); }

    operator git_object*() { return obj; }
    git_object** ptr() { return &obj; }
};

// RAII wrapper for git_commit
class GitCommit {
public:
    git_commit* commit = nullptr;

    GitCommit() = default;
    ~GitCommit() { if (commit) git_commit_free(commit); }

    operator git_commit*() { return commit; }
    git_commit** ptr() { return &commit; }
};

// RAII wrapper for git_tree
class GitTree {
public:
    git_tree* tree = nullptr;

    GitTree() = default;
    ~GitTree() { if (tree) git_tree_free(tree); }

    operator git_tree*() { return tree; }
    git_tree** ptr() { return &tree; }
};

GitWorker::GitWorker(QObject *parent)
    : QThread(parent)
    , m_running(true)
{
    qRegisterMetaType<GitTaskRequest>("GitTaskRequest");
    qRegisterMetaType<GitTaskResult>("GitTaskResult");

    git_libgit2_init();
}

GitWorker::~GitWorker()
{
    stopWorker();
    wait();
    git_libgit2_shutdown();
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

            if (!m_running) break;
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

QString GitWorker::getLastError()
{
    const git_error* err = git_error_last();
    if (err) {
        return QString::fromUtf8(err->message);
    }
    return "Unknown error";
}

QString GitWorker::getCurrentBranch(const QString& repoPath)
{
    GitRepo repo;
    if (!repo.open(repoPath)) return QString();

    GitRef head;
    if (git_repository_head(head.ptr(), repo) != 0) return QString();

    const char* name = nullptr;
    if (git_branch_name(&name, head) == 0) {
        return QString::fromUtf8(name);
    }

    return QString();
}

int GitWorker::getStashCount(const QString& repoPath)
{
    GitRepo repo;
    if (!repo.open(repoPath)) return 0;

    int count = 0;
    git_stash_foreach(repo, [](size_t, const char*, const git_oid*, void* payload) -> int {
        (*static_cast<int*>(payload))++;
        return 0;
    }, &count);

    return count;
}

bool GitWorker::hasUncommittedChanges(const QString& repoPath)
{
    GitRepo repo;
    if (!repo.open(repoPath)) return false;

    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;

    GitStatusList status;
    if (git_status_list_new(status.ptr(), repo, &opts) != 0) return false;

    return git_status_list_entrycount(status) > 0;
}

GitTaskResult GitWorker::handleCheckStatus(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    QVariantMap statusData;

    // Current branch
    QString branch = getCurrentBranch(req.repoPath);
    statusData["currentBranch"] = branch;

    // Check for uncommitted changes
    statusData["needsCommit"] = hasUncommittedChanges(req.repoPath);

    // Ahead/behind tracking
    int ahead = 0, behind = 0;

    GitRef head;
    if (git_repository_head(head.ptr(), repo) == 0) {
        GitRef upstream;
        if (git_branch_upstream(upstream.ptr(), head) == 0) {
            size_t a = 0, b = 0;
            const git_oid* local_oid = git_reference_target(head);
            const git_oid* upstream_oid = git_reference_target(upstream);

            if (local_oid && upstream_oid) {
                git_graph_ahead_behind(&a, &b, repo, local_oid, upstream_oid);
                ahead = static_cast<int>(a);
                behind = static_cast<int>(b);
            }
        }
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitRemote remote;
    if (git_remote_lookup(remote.ptr(), repo, "origin") != 0) {
        result.success = false;
        result.message = "No origin remote found";
        return result;
    }

    git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
    int auth_attempts = 0;
    opts.callbacks.credentials = credentials_callback;
    opts.callbacks.certificate_check = certificate_check_callback;
    opts.callbacks.payload = &auth_attempts;

    qDebug() << "Fetching from origin for" << req.repoPath;
    if (git_remote_fetch(remote, nullptr, &opts, nullptr) != 0) {
        result.success = false;
        result.message = getLastError();
        qDebug() << "Fetch failed:" << result.message;
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

    // First fetch
    GitTaskResult fetchResult = handleFetch(req);
    if (!fetchResult.success) {
        return fetchResult;
    }

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Get current branch and its upstream
    GitRef head;
    if (git_repository_head(head.ptr(), repo) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitRef upstream;
    if (git_branch_upstream(upstream.ptr(), head) != 0) {
        result.success = false;
        result.message = "No upstream branch configured";
        return result;
    }

    // Get the upstream commit
    const git_oid* upstream_oid = git_reference_target(upstream);
    if (!upstream_oid) {
        result.success = false;
        result.message = "Cannot get upstream target";
        return result;
    }

    git_annotated_commit* annotated = nullptr;
    if (git_annotated_commit_lookup(&annotated, repo, upstream_oid) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Perform merge analysis
    git_merge_analysis_t analysis;
    git_merge_preference_t preference;
    const git_annotated_commit* heads[] = { annotated };

    if (git_merge_analysis(&analysis, &preference, repo, heads, 1) != 0) {
        git_annotated_commit_free(annotated);
        result.success = false;
        result.message = getLastError();
        return result;
    }

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        git_annotated_commit_free(annotated);
        result.success = true;
        result.message = "Already up to date";
        return result;
    }

    if (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) {
        // Fast-forward
        GitRef new_ref;
        git_reference_set_target(new_ref.ptr(), head, upstream_oid, "pull: fast-forward");

        git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        git_checkout_head(repo, &checkout_opts);

        git_annotated_commit_free(annotated);
        result.success = true;
        result.message = "Fast-forward merge successful";
        return result;
    }

    if (analysis & GIT_MERGE_ANALYSIS_NORMAL) {
        // Normal merge
        git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
        git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

        if (git_merge(repo, heads, 1, &merge_opts, &checkout_opts) != 0) {
            git_annotated_commit_free(annotated);
            result.success = false;
            result.message = getLastError();
            return result;
        }

        // Check for conflicts
        GitIndex index;
        git_repository_index(index.ptr(), repo);
        if (git_index_has_conflicts(index)) {
            git_annotated_commit_free(annotated);
            result.success = false;
            result.message = "Merge conflicts - resolve manually";
            return result;
        }

        // Create merge commit
        GitSignature sig;
        git_signature_default(sig.ptr(), repo);

        git_oid tree_oid;
        git_index_write_tree(&tree_oid, index);

        GitTree tree;
        git_tree_lookup(tree.ptr(), repo, &tree_oid);

        GitCommit head_commit;
        git_commit_lookup(head_commit.ptr(), repo, git_reference_target(head));

        GitCommit upstream_commit;
        git_commit_lookup(upstream_commit.ptr(), repo, upstream_oid);

        const git_commit* parents[] = { head_commit, upstream_commit };
        git_oid new_commit_oid;
        git_commit_create(&new_commit_oid, repo, "HEAD", sig, sig,
                          nullptr, "Merge branch", tree, 2, parents);

        git_repository_state_cleanup(repo);
        git_annotated_commit_free(annotated);

        result.success = true;
        result.message = "Merge successful";
        return result;
    }

    git_annotated_commit_free(annotated);
    result.success = false;
    result.message = "Cannot merge";
    return result;
}

GitTaskResult GitWorker::handlePush(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitRemote remote;
    if (git_remote_lookup(remote.ptr(), repo, "origin") != 0) {
        result.success = false;
        result.message = "No origin remote found";
        return result;
    }

    // Get current branch
    GitRef head;
    if (git_repository_head(head.ptr(), repo) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    const char* branch_name = git_reference_shorthand(head);
    QString refspec = QString("refs/heads/%1:refs/heads/%1").arg(branch_name);
    QByteArray refspec_bytes = refspec.toUtf8();
    const char* refspec_str = refspec_bytes.constData();
    git_strarray refspecs = { const_cast<char**>(&refspec_str), 1 };

    git_push_options opts = GIT_PUSH_OPTIONS_INIT;
    int auth_attempts = 0;
    opts.callbacks.credentials = credentials_callback;
    opts.callbacks.certificate_check = certificate_check_callback;
    opts.callbacks.payload = &auth_attempts;

    qDebug() << "Pushing" << refspec << "to origin";
    if (git_remote_push(remote, &refspecs, &opts) != 0) {
        result.success = false;
        result.message = getLastError();
        qDebug() << "Push failed:" << result.message;
        return result;
    }

    qDebug() << "Push successful";
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitIndex index;
    if (git_repository_index(index.ptr(), repo) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Add files to index
    if (files.isEmpty()) {
        // Add all
        git_index_add_all(index, nullptr, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr);
    } else {
        for (const QString& file : files) {
            git_index_add_bypath(index, file.toUtf8().constData());
        }
    }

    if (git_index_write(index) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Write tree
    git_oid tree_oid;
    if (git_index_write_tree(&tree_oid, index) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitTree tree;
    if (git_tree_lookup(tree.ptr(), repo, &tree_oid) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Get signature
    GitSignature sig;
    if (git_signature_default(sig.ptr(), repo) != 0) {
        result.success = false;
        result.message = "Cannot create signature - configure user.name and user.email";
        return result;
    }

    // Get parent commit
    GitCommit parent;
    GitRef head;
    int has_parent = 0;
    if (git_repository_head(head.ptr(), repo) == 0) {
        git_commit_lookup(parent.ptr(), repo, git_reference_target(head));
        has_parent = 1;
    }

    const git_commit* parents[] = { parent };
    git_oid commit_oid;

    if (git_commit_create(&commit_oid, repo, "HEAD", sig, sig,
                          nullptr, message.toUtf8().constData(), tree,
                          has_parent, has_parent ? parents : nullptr) != 0) {
        result.success = false;
        result.message = getLastError();
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Stash if needed
    int stashCountBefore = getStashCount(req.repoPath);
    bool stashCreated = false;

    if (hasUncommittedChanges(req.repoPath)) {
        GitSignature sig;
        git_signature_default(sig.ptr(), repo);
        git_oid stash_oid;
        if (git_stash_save(&stash_oid, repo, sig, "auto-stash", GIT_STASH_DEFAULT) == 0) {
            stashCreated = true;
        }
    }

    // Try to find local branch
    GitRef branch;
    QString localRef = QString("refs/heads/%1").arg(branchName);

    if (git_reference_lookup(branch.ptr(), repo, localRef.toUtf8().constData()) != 0) {
        // Try to create from remote
        QString remoteRef = QString("refs/remotes/origin/%1").arg(branchName);
        GitRef remote_ref;

        if (git_reference_lookup(remote_ref.ptr(), repo, remoteRef.toUtf8().constData()) == 0) {
            // Create local branch from remote
            GitCommit commit;
            git_commit_lookup(commit.ptr(), repo, git_reference_target(remote_ref));

            git_branch_create(branch.ptr(), repo, branchName.toUtf8().constData(), commit, 0);

            // Set upstream
            git_branch_set_upstream(branch, QString("origin/%1").arg(branchName).toUtf8().constData());
        } else {
            if (stashCreated) {
                git_stash_pop(repo, 0, nullptr);
            }
            result.success = false;
            result.message = QString("Branch '%1' not found").arg(branchName);
            return result;
        }
    }

    // Checkout
    GitObject target;
    git_reference_peel(target.ptr(), branch, GIT_OBJECT_COMMIT);

    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if (git_checkout_tree(repo, target, &opts) != 0) {
        if (stashCreated) {
            git_stash_pop(repo, 0, nullptr);
        }
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Update HEAD
    git_repository_set_head(repo, localRef.toUtf8().constData());

    // Pop stash if created
    if (stashCreated) {
        if (git_stash_pop(repo, 0, nullptr) != 0) {
            result.message = QString("Switched to '%1' but stash pop failed").arg(branchName);
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Get HEAD commit
    GitRef head;
    if (git_repository_head(head.ptr(), repo) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitCommit commit;
    if (git_commit_lookup(commit.ptr(), repo, git_reference_target(head)) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitRef new_branch;
    if (git_branch_create(new_branch.ptr(), repo, branchName.toUtf8().constData(), commit, 0) != 0) {
        result.success = false;
        result.message = getLastError();
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Check if on this branch
    QString currentBranch = getCurrentBranch(req.repoPath);
    if (currentBranch == branchName) {
        // Checkout master/main first
        GitTaskRequest checkoutReq;
        checkoutReq.repoPath = req.repoPath;
        checkoutReq.args = QStringList() << "master";
        GitTaskResult checkoutResult = handleCheckout(checkoutReq);
        if (!checkoutResult.success) {
            checkoutReq.args = QStringList() << "main";
            handleCheckout(checkoutReq);
        }
    }

    GitRef branch;
    if (git_branch_lookup(branch.ptr(), repo, branchName.toUtf8().constData(), GIT_BRANCH_LOCAL) != 0) {
        result.success = false;
        result.message = "Branch not found";
        return result;
    }

    if (git_branch_delete(branch) != 0) {
        result.success = false;
        result.message = getLastError();
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Lookup source branch
    GitRef source;
    if (git_branch_lookup(source.ptr(), repo, sourceBranch.toUtf8().constData(), GIT_BRANCH_LOCAL) != 0) {
        result.success = false;
        result.message = "Source branch not found";
        return result;
    }

    git_annotated_commit* annotated = nullptr;
    if (git_annotated_commit_from_ref(&annotated, repo, source) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    git_merge_analysis_t analysis;
    git_merge_preference_t preference;
    const git_annotated_commit* heads[] = { annotated };

    git_merge_analysis(&analysis, &preference, repo, heads, 1);

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        git_annotated_commit_free(annotated);
        result.success = true;
        result.message = "Already up to date";
        return result;
    }

    git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if (git_merge(repo, heads, 1, &merge_opts, &checkout_opts) != 0) {
        git_annotated_commit_free(annotated);
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Check for conflicts
    GitIndex index;
    git_repository_index(index.ptr(), repo);
    if (git_index_has_conflicts(index)) {
        git_annotated_commit_free(annotated);
        result.success = false;
        result.message = "Merge conflicts - resolve manually";
        return result;
    }

    // Create merge commit
    GitSignature sig;
    git_signature_default(sig.ptr(), repo);

    git_oid tree_oid;
    git_index_write_tree(&tree_oid, index);

    GitTree tree;
    git_tree_lookup(tree.ptr(), repo, &tree_oid);

    GitRef head;
    git_repository_head(head.ptr(), repo);

    GitCommit head_commit;
    git_commit_lookup(head_commit.ptr(), repo, git_reference_target(head));

    GitCommit source_commit;
    git_commit_lookup(source_commit.ptr(), repo, git_reference_target(source));

    QString msg = QString("Merge branch '%1'").arg(sourceBranch);
    const git_commit* parents[] = { head_commit, source_commit };
    git_oid new_commit_oid;
    git_commit_create(&new_commit_oid, repo, "HEAD", sig, sig,
                      nullptr, msg.toUtf8().constData(), tree, 2, parents);

    git_repository_state_cleanup(repo);
    git_annotated_commit_free(annotated);

    // Auto-delete source if on master/main
    QString currentBranch = getCurrentBranch(req.repoPath);
    if (currentBranch == "master" || currentBranch == "main") {
        git_branch_delete(source);
    }

    result.success = true;
    result.message = "Merge successful";
    return result;
}

GitTaskResult GitWorker::handleReset(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitIndex index;
    if (git_repository_index(index.ptr(), repo) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    // Get HEAD tree
    GitRef head;
    git_repository_head(head.ptr(), repo);

    GitCommit commit;
    git_commit_lookup(commit.ptr(), repo, git_reference_target(head));

    GitTree tree;
    git_commit_tree(tree.ptr(), commit);

    // Reset index to HEAD
    if (git_index_read_tree(index, tree) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    git_index_write(index);

    result.success = true;
    result.message = "Reset successful";
    return result;
}

GitTaskResult GitWorker::handleRestore(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_FORCE;

    if (git_checkout_head(repo, &opts) != 0) {
        result.success = false;
        result.message = getLastError();
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    GitSignature sig;
    if (git_signature_default(sig.ptr(), repo) != 0) {
        result.success = false;
        result.message = "Cannot create signature";
        return result;
    }

    git_oid stash_oid;
    int err = git_stash_save(&stash_oid, repo, sig, nullptr, GIT_STASH_DEFAULT);

    if (err == GIT_ENOTFOUND) {
        result.success = true;
        result.data = false;
        result.message = "Nothing to stash";
        return result;
    }

    if (err != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    result.success = true;
    result.data = true;
    result.message = "Stash created";
    return result;
}

GitTaskResult GitWorker::handleStashPop(const GitTaskRequest& req)
{
    GitTaskResult result;
    result.requestId = req.requestId;

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    if (git_stash_pop(repo, 0, nullptr) != 0) {
        result.success = false;
        result.message = getLastError();
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    QStringList localBranches;
    QStringList remoteBranches;

    // Local branches
    git_branch_iterator* iter = nullptr;
    git_branch_iterator_new(&iter, repo, GIT_BRANCH_LOCAL);

    git_reference* ref = nullptr;
    git_branch_t type;
    while (git_branch_next(&ref, &type, iter) == 0) {
        const char* name = nullptr;
        git_branch_name(&name, ref);
        localBranches.append(QString::fromUtf8(name));
        git_reference_free(ref);
    }
    git_branch_iterator_free(iter);

    // Remote branches
    git_branch_iterator_new(&iter, repo, GIT_BRANCH_REMOTE);
    while (git_branch_next(&ref, &type, iter) == 0) {
        const char* name = nullptr;
        git_branch_name(&name, ref);
        QString branchName = QString::fromUtf8(name);
        // Remove "origin/" prefix
        if (branchName.startsWith("origin/")) {
            branchName = branchName.mid(7);
        }
        if (!branchName.contains("HEAD") && !localBranches.contains(branchName)) {
            remoteBranches.append(branchName);
        }
        git_reference_free(ref);
    }
    git_branch_iterator_free(iter);

    QString currentBranch = getCurrentBranch(req.repoPath);

    localBranches.sort(Qt::CaseInsensitive);
    remoteBranches.sort(Qt::CaseInsensitive);

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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    QStringList modifiedFiles;
    QStringList stagedFiles;

    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    GitStatusList status;
    if (git_status_list_new(status.ptr(), repo, &opts) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    size_t count = git_status_list_entrycount(status);
    for (size_t i = 0; i < count; i++) {
        const git_status_entry* entry = git_status_byindex(status, i);

        QString path;
        if (entry->index_to_workdir && entry->index_to_workdir->new_file.path) {
            path = QString::fromUtf8(entry->index_to_workdir->new_file.path);
        } else if (entry->head_to_index && entry->head_to_index->new_file.path) {
            path = QString::fromUtf8(entry->head_to_index->new_file.path);
        }

        if (path.isEmpty()) continue;

        // Filter patterns
        if (path.startsWith(".idea/") || path.contains("/__pycache__/") || path.startsWith("venv/")) {
            continue;
        }

        // Staged changes
        if (entry->status & (GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
                             GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_RENAMED)) {
            if (!stagedFiles.contains(path)) {
                stagedFiles.append(path);
            }
        }

        // Workdir changes (not staged)
        if (entry->status & (GIT_STATUS_WT_NEW | GIT_STATUS_WT_MODIFIED |
                             GIT_STATUS_WT_DELETED | GIT_STATUS_WT_RENAMED)) {
            if (!modifiedFiles.contains(path) && !stagedFiles.contains(path)) {
                modifiedFiles.append(path);
            }
        }
    }

    modifiedFiles.sort(Qt::CaseInsensitive);
    stagedFiles.sort(Qt::CaseInsensitive);

    QVariantMap changeData;
    changeData["modified"] = modifiedFiles;
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

    // Check for binary
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

    GitRepo repo;
    if (!repo.open(req.repoPath)) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    QByteArray pathBytes = filePath.toUtf8();
    char* pathStr = pathBytes.data();
    opts.pathspec.strings = &pathStr;
    opts.pathspec.count = 1;

    GitDiff diff;
    if (git_diff_index_to_workdir(diff.ptr(), repo, nullptr, &opts) != 0) {
        result.success = false;
        result.message = getLastError();
        return result;
    }

    QString diffOutput;

    git_diff_print(diff, GIT_DIFF_FORMAT_PATCH,
        [](const git_diff_delta*, const git_diff_hunk*,
           const git_diff_line* line, void* payload) -> int {
            QString* output = static_cast<QString*>(payload);
            if (line->content && line->content_len > 0) {
                *output += QString::fromUtf8(line->content, static_cast<int>(line->content_len));
            }
            return 0;
        }, &diffOutput);

    result.success = true;
    result.data = diffOutput;
    return result;
}
