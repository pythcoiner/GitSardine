#include "ChangesTreeWidget.h"
#include <QHeaderView>

ChangesTreeWidget::ChangesTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
    , m_allItem(nullptr)
    , m_ignoreChanges(false)
{
    setupUi();
}

void ChangesTreeWidget::setupUi()
{
    // Hide header
    header()->setVisible(false);

    // Single column
    setColumnCount(1);

    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &ChangesTreeWidget::onContextMenu);

    // Item changed signal
    connect(this, &QTreeWidget::itemChanged,
            this, &ChangesTreeWidget::onItemChanged);

    // Visual settings
    setRootIsDecorated(false);
    setIndentation(0);
}

void ChangesTreeWidget::setChanges(const QList<FileChange>& unstaged, const QList<FileChange>& staged)
{
    m_ignoreChanges = true;
    clear();
    m_fileItems.clear();
    m_allItem = nullptr;

    if (unstaged.isEmpty() && staged.isEmpty()) {
        m_ignoreChanges = false;
        return;
    }

    // Add modified files header
    if (!unstaged.isEmpty()) {
        QTreeWidgetItem* headerItem = new QTreeWidgetItem(this);
        headerItem->setText(0, "---- Modified files ----");
        headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsUserCheckable);

        // Add "-- All --" item
        m_allItem = new QTreeWidgetItem(this);
        m_allItem->setText(0, "-- All --");
        m_allItem->setFlags(m_allItem->flags() | Qt::ItemIsUserCheckable);
        m_allItem->setCheckState(0, Qt::Unchecked);

        // Add each file
        for (const FileChange& change : unstaged) {
            QTreeWidgetItem* item = new QTreeWidgetItem(this);
            item->setText(0, change.path);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, Qt::Unchecked);
            item->setToolTip(0, change.path);
            m_fileItems.append(item);
        }
    }

    // Add cached/staged files header
    if (!staged.isEmpty()) {
        QTreeWidgetItem* cachedHeader = new QTreeWidgetItem(this);
        cachedHeader->setText(0, "---- Cached files ----");
        cachedHeader->setFlags(cachedHeader->flags() & ~Qt::ItemIsUserCheckable);

        for (const FileChange& change : staged) {
            QTreeWidgetItem* item = new QTreeWidgetItem(this);
            item->setText(0, change.path);
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
            item->setToolTip(0, change.path);
        }
    }

    m_ignoreChanges = false;
}

void ChangesTreeWidget::clearChanges()
{
    m_ignoreChanges = true;
    clear();
    m_fileItems.clear();
    m_allItem = nullptr;
    m_ignoreChanges = false;
}

QStringList ChangesTreeWidget::getCheckedFiles() const
{
    QStringList files;
    for (QTreeWidgetItem* item : m_fileItems) {
        if (item->checkState(0) == Qt::Checked) {
            files.append(item->text(0));
        }
    }
    return files;
}

void ChangesTreeWidget::onItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_ignoreChanges || column != 0) {
        return;
    }

    if (item == m_allItem) {
        if (item->checkState(0) == Qt::Checked) {
            selectAllChanges();
        } else {
            deselectAllChanges();
        }
    }

    emit filesChecked(getCheckedFiles());
}

void ChangesTreeWidget::selectAllChanges()
{
    m_ignoreChanges = true;
    for (QTreeWidgetItem* item : m_fileItems) {
        item->setCheckState(0, Qt::Checked);
    }
    m_ignoreChanges = false;
}

void ChangesTreeWidget::deselectAllChanges()
{
    m_ignoreChanges = true;
    for (QTreeWidgetItem* item : m_fileItems) {
        item->setCheckState(0, Qt::Unchecked);
    }
    m_ignoreChanges = false;
}

void ChangesTreeWidget::onContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = itemAt(pos);
    if (!item || item == m_allItem) {
        return;
    }

    // Check if this is a file item (not a header)
    QString filename = item->text(0);
    if (filename.startsWith("----")) {
        return;
    }

    QMenu menu(this);
    QAction* diffAction = menu.addAction("Check diff");

    QAction* selected = menu.exec(mapToGlobal(pos));
    if (selected == diffAction) {
        emit diffRequested(filename);
    }
}
