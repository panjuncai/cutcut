#include "appcontroller.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/cutcut.png")));
    QApplication::setQuitOnLastWindowClosed(false);

    AppController controller;
    if (!controller.initialize()) {
        return 1;
    }

    return app.exec();
}
