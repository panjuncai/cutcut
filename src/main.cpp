#include "appcontroller.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/cutcut.png")));
    QApplication::setQuitOnLastWindowClosed(false);

    AppController controller;
    if (!controller.initialize()) {
        return 1;
    }

    return app.exec();
}
