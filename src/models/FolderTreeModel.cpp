#include "FolderTreeModel.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QPixmap>
#include "icons/icons.h"

// FolderItem implementation
FolderItem::FolderItem(const QString& name)
    : QStandardItem(name)
    , depth(0)
    , isRepo(false)
    , needsPull(false)
    , needsPush(false)
    , needsCommit(false)
    , statusError(false)
    , statusChecked(false)
{
    setEditable(false);
}

void FolderItem::updateIcon()
{
    QPixmap pixmap;

    if (isRepo) {
        if (statusError) {
            pixmap.loadFromData(icon_exclamation_red_png, icon_exclamation_red_png_len, "PNG");
        } else if (needsPull) {
            pixmap.loadFromData(icon_drive_download_png, icon_drive_download_png_len, "PNG");
        } else if (needsPush) {
            pixmap.loadFromData(icon_drive_upload_png, icon_drive_upload_png_len, "PNG");
        } else if (needsCommit) {
            pixmap.loadFromData(icon_disk_plus_png, icon_disk_plus_png_len, "PNG");
        } else if (statusChecked) {
            pixmap.loadFromData(icon_document_png, icon_document_png_len, "PNG");
        } else {
            // Default - not checked yet
            pixmap.loadFromData(icon_arrow_circle_315_png, icon_arrow_circle_315_png_len, "PNG");
        }
    } else if (depth == 0) {
        // Root drive
        pixmap.loadFromData(icon_drive_png, icon_drive_png_len, "PNG");
    } else {
        // Folder
        pixmap.loadFromData(icon_folder_horizontal_png, icon_folder_horizontal_png_len, "PNG");
    }

    setIcon(QIcon(pixmap));
}

// FolderTreeModel implementation
FolderTreeModel::FolderTreeModel(QObject *parent)
    : QStandardItemModel(parent)
{
    setColumnCount(1);
}

void FolderTreeModel::clearAll()
{
    clear();
    m_pathToItem.clear();
}

void FolderTreeModel::scanPaths(const QStringList& paths)
{
    clearAll();

    for (const QString& rootPath : paths) {
        QDir rootDir(rootPath);
        if (!rootDir.exists()) {
            continue;
        }

        // Create root item
        FolderItem* rootItem = new FolderItem(rootDir.dirName());
        rootItem->osPath = rootPath;
        rootItem->relativePath = rootDir.dirName();
        rootItem->depth = 0;
        rootItem->isRepo = QFileInfo::exists(rootDir.filePath(".git"));

        m_pathToItem[rootPath] = rootItem;
        invisibleRootItem()->appendRow(rootItem);

        // Scan subdirectories
        if (!rootItem->isRepo) {
            scanDirectory(rootPath, rootItem, 1);
        }

        // Update icon after scanning (to reflect if it has children)
        rootItem->updateIcon();
    }

    emit scanComplete();
}

bool FolderTreeModel::scanDirectory(const QString& path, FolderItem* parent, int depth)
{
    QDir dir(path);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    bool hasRepos = false;

    for (const QString& entry : entries) {
        // Skip hidden directories and common non-repo directories
        if (entry.startsWith(".") || entry == "node_modules" ||
            entry == "__pycache__" || entry == "venv" || entry == ".venv" ||
            entry == "build" || entry == "target" || entry == "dist") {
            continue;
        }

        QString fullPath = dir.filePath(entry);
        bool isGitRepo = QFileInfo::exists(QDir(fullPath).filePath(".git"));

        if (isGitRepo) {
            // Add repo directly
            FolderItem* item = new FolderItem(entry);
            item->osPath = fullPath;
            item->relativePath = entry;
            item->depth = depth;
            item->isRepo = true;
            item->updateIcon();

            m_pathToItem[fullPath] = item;
            parent->appendRow(item);
            hasRepos = true;
        } else {
            // Create temporary item to scan children
            FolderItem* item = new FolderItem(entry);
            item->osPath = fullPath;
            item->relativePath = entry;
            item->depth = depth;
            item->isRepo = false;

            // Recursively scan - only add if contains repos
            bool childHasRepos = scanDirectory(fullPath, item, depth + 1);

            if (childHasRepos) {
                item->updateIcon();
                m_pathToItem[fullPath] = item;
                parent->appendRow(item);
                hasRepos = true;
            } else {
                // No repos found, discard this folder
                delete item;
            }
        }
    }

    return hasRepos;
}

FolderItem* FolderTreeModel::getItemAt(const QModelIndex& index) const
{
    QStandardItem* item = itemFromIndex(index);
    return dynamic_cast<FolderItem*>(item);
}

FolderItem* FolderTreeModel::findByPath(const QString& path) const
{
    return m_pathToItem.value(path, nullptr);
}

void FolderTreeModel::updateRepoStatus(const QString& path, const RepoStatus& status)
{
    FolderItem* item = findByPath(path);
    if (!item || !item->isRepo) {
        return;
    }

    item->needsPull = status.needsPull;
    item->needsPush = status.needsPush;
    item->needsCommit = status.needsCommit;
    item->statusError = status.hasError;
    item->statusChecked = true;
    item->updateIcon();
}

QStringList FolderTreeModel::getAllRepoPaths() const
{
    QStringList paths;
    for (auto it = m_pathToItem.constBegin(); it != m_pathToItem.constEnd(); ++it) {
        if (it.value()->isRepo) {
            paths.append(it.key());
        }
    }
    return paths;
}

FolderItem* FolderTreeModel::getOrCreateFolder(const QString& path, FolderItem* parent)
{
    if (m_pathToItem.contains(path)) {
        return m_pathToItem[path];
    }

    QDir dir(path);
    FolderItem* item = new FolderItem(dir.dirName());
    item->osPath = path;
    item->relativePath = dir.dirName();
    item->depth = parent ? parent->depth + 1 : 0;
    item->isRepo = false;
    item->updateIcon();

    m_pathToItem[path] = item;

    if (parent) {
        parent->appendRow(item);
    } else {
        invisibleRootItem()->appendRow(item);
    }

    return item;
}
