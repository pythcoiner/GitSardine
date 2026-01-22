#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QMainWindow>
#include <QTimer>

#include "config/Config.h"
#include "models/FolderTreeModel.h"
#include "workers/GitWorker.h"
#include "screens/MainScreen.h"

/**
 * AppController - Application orchestration and startup
 */
class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    bool initialize();
    void show();

private slots:
    void onAutoUpdate();

private:
    Config m_config;
    FolderTreeModel* m_folderModel;
    GitWorker* m_gitWorker;
    MainScreen* m_mainScreen;
    QMainWindow* m_mainWindow;
    QTimer* m_autoUpdateTimer;

    bool loadConfig();
    void setupMainWindow();
    void startAutoUpdate();
    void applyDarkTheme();
};

#endif // APPCONTROLLER_H
