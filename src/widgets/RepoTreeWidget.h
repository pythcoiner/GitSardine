#ifndef REPOTREEWIDGET_H
#define REPOTREEWIDGET_H

#include <QTreeView>
#include <QTimer>
#include "models/FolderTreeModel.h"
#include "git/GitStatus.h"

/**
 * RepoTreeWidget - Tree view for repository folders
 */
class RepoTreeWidget : public QTreeView {
    Q_OBJECT

public:
    explicit RepoTreeWidget(QWidget *parent = nullptr);

    void setFolderModel(FolderTreeModel* model);
    FolderTreeModel* folderModel() const { return m_model; }

signals:
    void repoSelected(const QString& path);
    void refreshRequested();

public slots:
    void updateRepoStatus(const QString& path, const RepoStatus& status);
    void setSpinnerActive(bool active);
    void expandAllItems();

private slots:
    void onClicked(const QModelIndex& index);
    void onDoubleClicked(const QModelIndex& index);
    void updateSpinner();

private:
    FolderTreeModel* m_model;
    QTimer* m_spinnerTimer;
    int m_spinnerFrame;

    void setupUi();
};

#endif // REPOTREEWIDGET_H
