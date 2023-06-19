#include "mainwindow.h"

#include <QApplication>

#include <windows.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(GetCommandLine());
    w.show();
    a.exec();
    return w.exitCode();
}
