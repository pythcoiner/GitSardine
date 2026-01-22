#ifndef GITSTATUS_H
#define GITSTATUS_H

#include <QString>
#include <QList>

/**
 * File status in a git repository
 */
enum class FileStatus {
    Modified,       // Modified in working tree
    Staged,         // Staged for commit
    Untracked,      // Not tracked by git
    Deleted,        // Deleted from working tree
    Renamed,        // Renamed
    Conflicted      // Merge conflict
};

/**
 * Repository status summary
 */
struct RepoStatus {
    bool needsPull = false;
    bool needsPush = false;
    bool needsCommit = false;
    bool hasError = false;
    QString errorMessage;
    QString currentBranch;
    int ahead = 0;
    int behind = 0;

    bool isClean() const {
        return !needsPull && !needsPush && !needsCommit && !hasError;
    }
};

/**
 * Single file change entry
 */
struct FileChange {
    QString path;
    FileStatus status;
    bool isStaged = false;

    FileChange() : status(FileStatus::Modified) {}
    FileChange(const QString& p, FileStatus s, bool staged = false)
        : path(p), status(s), isStaged(staged) {}
};

#endif // GITSTATUS_H
