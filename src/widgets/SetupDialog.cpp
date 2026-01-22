#include "SetupDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QMessageBox>
#include <QApplication>
#include <QPixmap>
#include "icons/icons.h"

SetupDialog::SetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("GitSardine Setup");
    setModal(true);
    setMinimumWidth(450);

    auto *mainLayout = new QVBoxLayout(this);

    // Username section
    auto *usernameLabel = new QLabel("Username:", this);
    m_usernameInput = new QLineEdit(this);
    m_usernameInput->setPlaceholderText("Enter your username");
    mainLayout->addWidget(usernameLabel);
    mainLayout->addWidget(m_usernameInput);

    mainLayout->addSpacing(10);

    // Directories section
    auto *pathsLabel = new QLabel("Directories containing git repositories:", this);
    mainLayout->addWidget(pathsLabel);

    m_pathsList = new QListWidget(this);
    m_pathsList->setMinimumHeight(150);
    mainLayout->addWidget(m_pathsList);

    // Add/Remove buttons
    auto *buttonsLayout = new QHBoxLayout();
    m_addPathBtn = new QPushButton("Add Directory", this);
    m_removePathBtn = new QPushButton("Remove", this);
    m_removePathBtn->setEnabled(false);
    buttonsLayout->addWidget(m_addPathBtn);
    buttonsLayout->addWidget(m_removePathBtn);
    buttonsLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);

    // Validation label
    m_validationLabel = new QLabel(this);
    m_validationLabel->setWordWrap(true);
    m_validationLabel->setStyleSheet("QLabel { color: #ff6b6b; }");
    mainLayout->addWidget(m_validationLabel);

    mainLayout->addSpacing(10);

    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    // Connections
    connect(m_addPathBtn, &QPushButton::clicked, this, &SetupDialog::onAddPath);
    connect(m_removePathBtn, &QPushButton::clicked, this, &SetupDialog::onRemovePath);
    connect(m_usernameInput, &QLineEdit::textChanged, this, &SetupDialog::validateInput);
    connect(m_pathsList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removePathBtn->setEnabled(m_pathsList->currentItem() != nullptr);
    });
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Initial validation
    validateInput();
}

QString SetupDialog::username() const
{
    return m_usernameInput->text().trimmed();
}

QStringList SetupDialog::paths() const
{
    QStringList result;
    for (int i = 0; i < m_pathsList->count(); ++i) {
        // Get the actual path from UserRole data
        QString path = m_pathsList->item(i)->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            result.append(path);
        }
    }
    return result;
}

void SetupDialog::onAddPath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Directory Containing Git Repositories",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        // Check for duplicates
        for (int i = 0; i < m_pathsList->count(); ++i) {
            QString existingPath = m_pathsList->item(i)->data(Qt::UserRole).toString();
            if (existingPath == dir) {
                return;  // Already in list
            }
        }

        // Check if this is a repo itself (not a parent directory)
        if (isGitRepo(dir)) {
            QMessageBox::warning(this, "Invalid Selection",
                "The selected directory is itself a git repository.\n\n"
                "Please select a parent directory that CONTAINS git repositories.");
            return;
        }

        // Show progress dialog while scanning
        ScanProgressDialog progress(dir, this);
        progress.show();
        QApplication::processEvents();

        int repoCount = countGitRepos(dir, &progress);

        progress.hide();

        if (repoCount == 0) {
            QMessageBox::warning(this, "No Repositories Found",
                "The selected directory does not contain any git repositories.\n\n"
                "Please select a directory that contains one or more git repositories.");
            return;
        }

        m_pathsList->addItem(QString("%1 (%2 repos)").arg(dir).arg(repoCount));
        m_pathsList->item(m_pathsList->count() - 1)->setData(Qt::UserRole, dir);
        validateInput();
    }
}

void SetupDialog::onRemovePath()
{
    auto *current = m_pathsList->currentItem();
    if (current) {
        delete m_pathsList->takeItem(m_pathsList->row(current));
        validateInput();
    }
}

void SetupDialog::validateInput()
{
    QString user = m_usernameInput->text().trimmed();
    QStringList errors;

    if (user.isEmpty()) {
        errors << "Enter a username";
    } else if (user == "user") {
        errors << "Username cannot be 'user'";
    }

    if (m_pathsList->count() == 0) {
        errors << "Add at least one directory containing git repositories";
    }

    bool valid = errors.isEmpty();
    m_validationLabel->setText(errors.join(" | "));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

bool SetupDialog::isGitRepo(const QString& path) const
{
    return QFileInfo::exists(QDir(path).filePath(".git"));
}

int SetupDialog::countGitRepos(const QString& path, ScanProgressDialog* progress) const
{
    int count = 0;
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString dir = it.next();
        // Skip common non-repo directories
        QString name = QFileInfo(dir).fileName();
        if (name.startsWith(".") || name == "node_modules" ||
            name == "__pycache__" || name == "venv" || name == ".venv") {
            continue;
        }

        if (QFileInfo::exists(QDir(dir).filePath(".git"))) {
            count++;
            if (progress) {
                progress->incrementCount();
            }
        }

        // Process events to keep UI responsive
        QApplication::processEvents();
    }

    return count;
}

// ScanProgressDialog implementation
ScanProgressDialog::ScanProgressDialog(const QString& path, QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_spinnerFrame(1)
    , m_repoCount(0)
{
    setModal(true);
    setFixedSize(300, 100);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    // Spinner and message in horizontal layout
    auto *topLayout = new QHBoxLayout();
    topLayout->setAlignment(Qt::AlignCenter);

    m_spinnerLabel = new QLabel(this);
    QPixmap spinnerPixmap;
    spinnerPixmap.loadFromData(icon_spin_1_png, icon_spin_1_png_len, "PNG");
    m_spinnerLabel->setPixmap(spinnerPixmap);
    topLayout->addWidget(m_spinnerLabel);

    m_messageLabel = new QLabel("Scanning for repositories...", this);
    topLayout->addWidget(m_messageLabel);

    layout->addLayout(topLayout);

    // Count label
    m_countLabel = new QLabel("Found: 0 repositories", this);
    m_countLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_countLabel);

    // Path being scanned
    QLabel *pathLabel = new QLabel(this);
    pathLabel->setText(QFileInfo(path).fileName());
    pathLabel->setAlignment(Qt::AlignCenter);
    pathLabel->setStyleSheet("color: #888888; font-size: 10px;");
    layout->addWidget(pathLabel);

    // Spinner animation timer
    m_spinnerTimer = new QTimer(this);
    m_spinnerTimer->setInterval(150);
    connect(m_spinnerTimer, &QTimer::timeout, this, &ScanProgressDialog::updateSpinner);
    m_spinnerTimer->start();

    // Style
    setStyleSheet("QDialog { background-color: #353535; border: 1px solid #555555; border-radius: 5px; }");
}

void ScanProgressDialog::incrementCount()
{
    m_repoCount++;
    m_countLabel->setText(QString("Found: %1 repositories").arg(m_repoCount));
}

void ScanProgressDialog::updateSpinner()
{
    m_spinnerFrame++;
    if (m_spinnerFrame > 4) {
        m_spinnerFrame = 1;
    }

    QPixmap spinnerPixmap;
    switch (m_spinnerFrame) {
        case 1: spinnerPixmap.loadFromData(icon_spin_1_png, icon_spin_1_png_len, "PNG"); break;
        case 2: spinnerPixmap.loadFromData(icon_spin_2_png, icon_spin_2_png_len, "PNG"); break;
        case 3: spinnerPixmap.loadFromData(icon_spin_3_png, icon_spin_3_png_len, "PNG"); break;
        case 4: spinnerPixmap.loadFromData(icon_spin_4_png, icon_spin_4_png_len, "PNG"); break;
    }
    m_spinnerLabel->setPixmap(spinnerPixmap);
}
