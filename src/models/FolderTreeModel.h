#ifndef FOLDERTREEMODEL_H
#define FOLDERTREEMODEL_H

#include <QStandardItemModel>
#include <QStandardItem>
#include <QString>
#include <QStringList>
#include <QHash>
#include "git/GitStatus.h"

/**
 * FolderItem - Custom QStandardItem for folder tree
 */
class FolderItem : public QStandardItem {
public:
    FolderItem(const QString& name);

    QString osPath;           // Full filesystem path
    QString relativePath;     // Relative path from root
    int depth;                // Nesting level
    bool isRepo;              // Is this a git repository?

    // Repository status (only valid if isRepo)
    bool needsPull;
    bool needsPush;
    bool needsCommit;
    bool statusError;
    bool statusChecked;

    void updateIcon();
};

/**
 * FolderTreeModel - Tree structure model for repository discovery
 */
class FolderTreeModel : public QStandardItemModel {
    Q_OBJECT

public:
    explicit FolderTreeModel(QObject *parent = nullptr);

    // Scan paths for repositories
    void scanPaths(const QStringList& paths);

    // Get repository model at index
    FolderItem* getItemAt(const QModelIndex& index) const;

    // Find item by path
    FolderItem* findByPath(const QString& path) const;

    // Update status for a repository
    void updateRepoStatus(const QString& path, const RepoStatus& status);

    // Get all repository paths
    QStringList getAllRepoPaths() const;

    // Clear all items
    void clearAll();

signals:
    void scanProgress(int current, int total);
    void scanComplete();

private:
    QHash<QString, FolderItem*> m_pathToItem;

    bool scanDirectory(const QString& path, FolderItem* parent, int depth);
    FolderItem* getOrCreateFolder(const QString& path, FolderItem* parent);
};

#endif // FOLDERTREEMODEL_H
