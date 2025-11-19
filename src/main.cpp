#include "appcontroller.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);

    AppController controller;
    if (!controller.initialize()) {
        return 1;
    }

    return app.exec();
}
