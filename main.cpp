#include "MainWindow.h"
#include <QApplication>

#include <memory>
using std::unique_ptr;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    auto w = MainWindow::make();
    w->show();

    return a.exec();
}
