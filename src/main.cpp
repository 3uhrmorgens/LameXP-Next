#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    MainWindow window;
    window.show();
    return app.exec();
}
