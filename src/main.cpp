#include <QApplication>
#include "controller/AppController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

    // Set application metadata
    QApplication::setApplicationName("GitSardine");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("GitSardine");

    // Create and initialize app controller
    AppController controller;
    if (!controller.initialize()) {
        return 1;
    }

    // Show main window
    controller.show();

    // Run event loop
    return app.exec();
}
