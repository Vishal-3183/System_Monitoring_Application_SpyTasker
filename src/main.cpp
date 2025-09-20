#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("Spy_Tasker");
    window.setWindowIcon(QIcon(":/logo/logo.png"));

    window.resize(1000, 600);
    window.show();

    return app.exec();
}
