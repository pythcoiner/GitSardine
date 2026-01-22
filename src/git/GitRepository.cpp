#include "GitRepository.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>

GitRepository::GitRepository(const QString& path)
    : m_path(path)
    , m_isValid(false)
{
    // Check if this is a valid git repository
    QString gitDir = getGitDir();
    m_isValid = QDir(gitDir).exists();
}

GitRepository::~GitRepository()
{
}

QString GitRepository::name() const
{
    return QDir(m_path).dirName();
}

QString GitRepository::getGitDir() const
{
    return QDir(m_path).filePath(".git");
}

QString GitRepository::readFile(const QString& relativePath) const
{
    QString fullPath = QDir(getGitDir()).filePath(relativePath);
    QFile file(fullPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);
    QString content = stream.readAll().trimmed();
    file.close();
    return content;
}

bool GitRepository::isGitRepository(const QString& path)
{
    return QDir(QDir(path).filePath(".git")).exists();
}

Result<QString, QString> GitRepository::getCurrentBranch() const
{
    if (!m_isValid) {
        return Result<QString, QString>::Err("Invalid repository");
    }

    QString head = readFile("HEAD");
    if (head.isEmpty()) {
        return Result<QString, QString>::Err("Cannot read HEAD");
    }

    // HEAD format: "ref: refs/heads/branch_name"
    if (head.startsWith("ref: refs/heads/")) {
        QString branch = head.mid(16);  // Remove "ref: refs/heads/"
        return Result<QString, QString>::Ok(branch);
    }

    // Detached HEAD state (raw commit hash)
    return Result<QString, QString>::Err("Detached HEAD - checkout a branch");
}

Result<QStringList, QString> GitRepository::getLocalBranches() const
{
    if (!m_isValid) {
        return Result<QStringList, QString>::Err("Invalid repository");
    }

    QStringList branches;
    QString headsPath = QDir(getGitDir()).filePath("refs/heads");
    QDir headsDir(headsPath);

    if (!headsDir.exists()) {
        return Result<QStringList, QString>::Ok(branches);
    }

    QDirIterator it(headsPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString relativePath = headsDir.relativeFilePath(it.filePath());
        branches.append(relativePath);
    }

    branches.sort(Qt::CaseInsensitive);
    return Result<QStringList, QString>::Ok(branches);
}

Result<QStringList, QString> GitRepository::getRemoteBranches() const
{
    if (!m_isValid) {
        return Result<QStringList, QString>::Err("Invalid repository");
    }

    QStringList branches;
    QString remotesPath = QDir(getGitDir()).filePath("refs/remotes");
    QDir remotesDir(remotesPath);

    if (!remotesDir.exists()) {
        return Result<QStringList, QString>::Ok(branches);
    }

    QDirIterator it(remotesPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString relativePath = remotesDir.relativeFilePath(it.filePath());
        // Skip HEAD references
        if (!relativePath.endsWith("/HEAD")) {
            branches.append(relativePath);
        }
    }

    branches.sort(Qt::CaseInsensitive);
    return Result<QStringList, QString>::Ok(branches);
}

Result<RepoStatus, QString> GitRepository::getStatus() const
{
    if (!m_isValid) {
        return Result<RepoStatus, QString>::Err("Invalid repository");
    }

    RepoStatus status;

    // Get current branch
    auto branchResult = getCurrentBranch();
    if (branchResult.isOk()) {
        status.currentBranch = branchResult.value();
    } else {
        status.hasError = true;
        status.errorMessage = branchResult.error();
        return Result<RepoStatus, QString>::Ok(status);
    }

    // Check for modified files (basic check - full status requires libgit2)
    auto changesResult = getChanges();
    if (changesResult.isOk() && !changesResult.value().isEmpty()) {
        status.needsCommit = true;
    }

    return Result<RepoStatus, QString>::Ok(status);
}

Result<QList<FileChange>, QString> GitRepository::getChanges() const
{
    // This is a placeholder - full implementation requires GitWorker
    // to properly use libgit2 for status
    QList<FileChange> changes;
    return Result<QList<FileChange>, QString>::Ok(changes);
}

Result<QList<FileChange>, QString> GitRepository::getCachedChanges() const
{
    // This is a placeholder - full implementation requires GitWorker
    QList<FileChange> changes;
    return Result<QList<FileChange>, QString>::Ok(changes);
}
