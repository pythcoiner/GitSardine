#ifndef CHANGESTREEWIDGET_H
#define CHANGESTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QList>
#include <QMenu>
#include "git/GitStatus.h"

/**
 * ChangesTreeWidget - Tree widget for displaying file changes
 */
class ChangesTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit ChangesTreeWidget(QWidget *parent = nullptr);

    QStringList getCheckedFiles() const;

signals:
    void filesChecked(const QStringList& files);
    void diffRequested(const QString& file);

public slots:
    void setChanges(const QList<FileChange>& unstaged, const QList<FileChange>& staged);
    void clearChanges();

private slots:
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onContextMenu(const QPoint& pos);

private:
    QTreeWidgetItem* m_allItem;
    QList<QTreeWidgetItem*> m_fileItems;
    bool m_ignoreChanges;

    void setupUi();
    void selectAllChanges();
    void deselectAllChanges();
};

#endif // CHANGESTREEWIDGET_H
