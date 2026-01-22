#include "AppController.h"
#include "widgets/SetupDialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <QPalette>
#include <QDir>

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_folderModel(nullptr)
    , m_gitWorker(nullptr)
    , m_mainScreen(nullptr)
    , m_mainWindow(nullptr)
    , m_autoUpdateTimer(nullptr)
{
}

AppController::~AppController()
{
    if (m_gitWorker) {
        m_gitWorker->stopWorker();
        m_gitWorker->wait();
        delete m_gitWorker;
    }

    delete m_folderModel;
    delete m_mainScreen;
    delete m_mainWindow;
}

bool AppController::initialize()
{
    // Apply dark theme
    applyDarkTheme();

    // Load or create config
    if (!loadConfig()) {
        return false;
    }

    // Create folder model and scan paths
    m_folderModel = new FolderTreeModel(this);

    // Filter out non-existent paths
    QStringList validPaths;
    for (const QString& path : m_config.paths) {
        if (QDir(path).exists()) {
            validPaths.append(path);
        } else {
            QMessageBox::warning(nullptr, "Invalid Path",
                QString("The following path does not exist and will be skipped:\n%1").arg(path));
        }
    }

    m_folderModel->scanPaths(validPaths);

    // Create git worker thread
    m_gitWorker = new GitWorker();
    m_gitWorker->start();

    // Setup main window
    setupMainWindow();

    // Start auto-update timer
    startAutoUpdate();

    return true;
}

bool AppController::loadConfig()
{
    if (!Config::exists()) {
        // Show setup dialog for first-run configuration
        SetupDialog dialog;
        if (dialog.exec() != QDialog::Accepted) {
            return false;  // User cancelled
        }

        // Set config from dialog values
        m_config.user = dialog.username();
        m_config.paths = dialog.paths();
        m_config.extend = 190;
        m_config.ignore = QVariantList();

        // Save the new config
        auto saveResult = m_config.save();
        if (saveResult.isErr()) {
            QMessageBox::critical(nullptr, "Configuration Error",
                QString("Failed to save config: %1").arg(saveResult.error()));
            return false;
        }

        return true;
    }

    auto result = m_config.load();
    if (result.isErr()) {
        QMessageBox::critical(nullptr, "Configuration Error",
            QString("Failed to load config: %1").arg(result.error()));
        return false;
    }

    return true;
}

void AppController::setupMainWindow()
{
    m_mainWindow = new QMainWindow();
    m_mainWindow->setWindowTitle("GitSardine");
    m_mainWindow->setFixedSize(290, 700);

    m_mainScreen = new MainScreen();
    m_mainScreen->setFolderModel(m_folderModel);
    m_mainScreen->setGitWorker(m_gitWorker);

    m_mainWindow->setCentralWidget(m_mainScreen);
}

void AppController::show()
{
    if (m_mainWindow) {
        m_mainWindow->show();
        // Trigger initial status update
        m_mainScreen->updateAllRepoStatus();
    }
}

void AppController::startAutoUpdate()
{
    m_autoUpdateTimer = new QTimer(this);
    m_autoUpdateTimer->setInterval(1800000);  // 30 minutes
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &AppController::onAutoUpdate);
    m_autoUpdateTimer->start();
}

void AppController::onAutoUpdate()
{
    if (m_mainScreen) {
        m_mainScreen->updateAllRepoStatus();
    }
}

void AppController::applyDarkTheme()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);

    qApp->setStyleSheet(
        "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }"
        "QTreeView { background-color: #191919; }"
        "QTreeView::item:hover { background-color: #353535; }"
        "QTreeView::item:selected { background-color: #2a82da; }"
        "QComboBox { background-color: #353535; }"
        "QLineEdit { background-color: #191919; border: 1px solid #353535; padding: 2px; }"
        "QPushButton { background-color: #353535; border: 1px solid #555555; padding: 4px; }"
        "QPushButton:hover { background-color: #454545; }"
        "QPushButton:pressed { background-color: #2a82da; }"
        "QPushButton:disabled { background-color: #252525; color: #555555; }"
        "QProgressBar { border: 1px solid #555555; background-color: #191919; text-align: center; }"
        "QProgressBar::chunk { background-color: #2a82da; }"
    );
}
