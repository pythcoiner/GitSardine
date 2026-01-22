#ifndef BRANCHSELECTOR_H
#define BRANCHSELECTOR_H

#include <QWidget>
#include <QComboBox>
#include <QHBoxLayout>
#include <QStringList>

/**
 * BranchSelector - Branch selection dropdown
 */
class BranchSelector : public QWidget {
    Q_OBJECT

public:
    explicit BranchSelector(QWidget *parent = nullptr);

    QString currentBranch() const;

signals:
    void branchChanged(const QString& branch);
    void newBranchRequested(const QString& name);

public slots:
    void setBranches(const QStringList& local, const QStringList& remote, const QString& current);
    void setEnabled(bool enabled);
    void clear();

private slots:
    void onComboChanged(int index);

private:
    QComboBox* m_combo;
    QString m_currentBranch;
    bool m_ignoreChanges;

    void setupUi();
};

#endif // BRANCHSELECTOR_H
