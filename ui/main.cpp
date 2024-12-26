#include "XPlayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char * argv[])
{
    QApplication a(argc, argv);
    XPlayer w;

    w.show();
    return a.exec();
}
