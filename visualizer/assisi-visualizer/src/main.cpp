#include "visualizer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Visualizer v("dummy.cfg");

    v.show();

    return a.exec();
}
