#ifndef GITREPOSITORY_H
#define GITREPOSITORY_H

#include <QString>
#include <QStringList>
#include <QList>
#include "core/Result.h"
#include "git/GitStatus.h"

/**
 * GitRepository - High-level wrapper for git operations
 *
 * Note: For thread safety, all libgit2 operations should go through
 * GitWorker. This class is for lightweight, read-only operations
 * that can be called from the UI thread without blocking.
 */
class GitRepository {
public:
    explicit GitRepository(const QString& path);
    ~GitRepository();

    // Basic queries (safe to call from any thread)
    bool isValid() const { return m_isValid; }
    QString path() const { return m_path; }
    QString name() const;

    // Branch operations
    Result<QString, QString> getCurrentBranch() const;
    Result<QStringList, QString> getLocalBranches() const;
    Result<QStringList, QString> getRemoteBranches() const;

    // Status operations
    Result<RepoStatus, QString> getStatus() const;
    Result<QList<FileChange>, QString> getChanges() const;
    Result<QList<FileChange>, QString> getCachedChanges() const;

    // Static helper to check if path is a git repository
    static bool isGitRepository(const QString& path);

private:
    QString m_path;
    bool m_isValid;

    QString readFile(const QString& relativePath) const;
    QString getGitDir() const;
};

#endif // GITREPOSITORY_H
