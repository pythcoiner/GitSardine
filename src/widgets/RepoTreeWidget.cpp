#include "RepoTreeWidget.h"
#include <QHeaderView>
#include "icons/icons.h"

RepoTreeWidget::RepoTreeWidget(QWidget *parent)
    : QTreeView(parent)
    , m_model(nullptr)
    , m_spinnerFrame(1)
{
    setupUi();
}

void RepoTreeWidget::setupUi()
{
    // Hide header
    header()->setVisible(false);

    // Enable tree expansion
    setRootIsDecorated(true);
    setItemsExpandable(true);
    setExpandsOnDoubleClick(false);  // We handle double-click manually

    // Set indentation for clear hierarchy display
    setIndentation(20);

    // Enable animated expansion
    setAnimated(true);

    // Selection mode
    setSelectionMode(QAbstractItemView::SingleSelection);

    // Allow clicking on items
    setAllColumnsShowFocus(true);

    // Connect signals
    connect(this, &QTreeView::doubleClicked, this, &RepoTreeWidget::onDoubleClicked);
    connect(this, &QTreeView::clicked, this, &RepoTreeWidget::onClicked);

    // Setup spinner timer
    m_spinnerTimer = new QTimer(this);
    m_spinnerTimer->setInterval(200);  // 200ms per frame
    connect(m_spinnerTimer, &QTimer::timeout, this, &RepoTreeWidget::updateSpinner);
}

void RepoTreeWidget::setFolderModel(FolderTreeModel* model)
{
    m_model = model;
    setModel(model);
}

void RepoTreeWidget::onClicked(const QModelIndex& index)
{
    if (!m_model) return;

    FolderItem* item = m_model->getItemAt(index);
    if (!item) return;

    if (item->isRepo) {
        emit repoSelected(item->osPath);
    }
}

void RepoTreeWidget::onDoubleClicked(const QModelIndex& index)
{
    if (!m_model) return;

    FolderItem* item = m_model->getItemAt(index);
    if (!item) return;

    if (item->isRepo) {
        emit repoSelected(item->osPath);
    } else {
        // Toggle expand/collapse for folders
        if (isExpanded(index)) {
            collapse(index);
        } else {
            expand(index);
        }
    }
}

void RepoTreeWidget::updateRepoStatus(const QString& path, const RepoStatus& status)
{
    if (m_model) {
        m_model->updateRepoStatus(path, status);
    }
}

void RepoTreeWidget::setSpinnerActive(bool active)
{
    if (active) {
        m_spinnerFrame = 1;
        m_spinnerTimer->start();
    } else {
        m_spinnerTimer->stop();
    }
}

void RepoTreeWidget::updateSpinner()
{
    m_spinnerFrame++;
    if (m_spinnerFrame > 4) {
        m_spinnerFrame = 1;
    }
    // The spinner icon update would be handled by the button, not the tree
}

void RepoTreeWidget::expandAllItems()
{
    expandAll();
}
