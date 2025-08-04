#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QCoreApplication::setOrganizationName("317 Lab");
    QCoreApplication::setOrganizationDomain("dartmouth.edu/~aurora");
    QCoreApplication::setApplicationName("317 Plotter");
    w.resize(600,600);
    w.show();
    return a.exec();
}
