#include "BranchSelector.h"

BranchSelector::BranchSelector(QWidget *parent)
    : QWidget(parent)
    , m_ignoreChanges(false)
{
    setupUi();
}

void BranchSelector::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_combo = new QComboBox(this);
    m_combo->setMinimumWidth(150);
    m_combo->setMinimumHeight(25);

    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BranchSelector::onComboChanged);

    layout->addWidget(m_combo);
    setLayout(layout);
}

QString BranchSelector::currentBranch() const
{
    return m_currentBranch;
}

void BranchSelector::setBranches(const QStringList& local, const QStringList& remote, const QString& current)
{
    m_ignoreChanges = true;
    m_currentBranch = current;

    m_combo->clear();

    // Add current branch first
    if (!current.isEmpty()) {
        m_combo->addItem(current);
    }

    // Add other local branches
    for (const QString& branch : local) {
        if (branch != current) {
            m_combo->addItem(branch);
        }
    }

    // Add remote-only branches with angle brackets
    for (const QString& branch : remote) {
        // Extract branch name from "origin/branch" format
        QString branchName = branch;
        if (branchName.startsWith("origin/")) {
            branchName = branchName.mid(7);
        }

        // Only add if not already in local branches
        if (!local.contains(branchName) && branchName != current) {
            m_combo->addItem(QString("<%1>").arg(branchName));
        }
    }

    // Add --new-- option
    m_combo->addItem("--new--");

    // Select current branch
    m_combo->setCurrentIndex(0);

    m_ignoreChanges = false;
}

void BranchSelector::setEnabled(bool enabled)
{
    m_combo->setEnabled(enabled);
}

void BranchSelector::clear()
{
    m_ignoreChanges = true;
    m_combo->clear();
    m_currentBranch.clear();
    m_ignoreChanges = false;
}

void BranchSelector::onComboChanged(int index)
{
    if (m_ignoreChanges || index < 0) {
        return;
    }

    QString selected = m_combo->currentText();

    if (selected == "--new--") {
        emit newBranchRequested(QString());
        // Reset to previous selection
        m_ignoreChanges = true;
        m_combo->setCurrentIndex(0);
        m_ignoreChanges = false;
    } else if (selected != m_currentBranch) {
        QString branchName = selected;

        // Remove angle brackets for remote branches
        if (branchName.startsWith("<") && branchName.endsWith(">")) {
            branchName = branchName.mid(1, branchName.length() - 2);
        }

        m_currentBranch = branchName;
        emit branchChanged(branchName);
    }
}
