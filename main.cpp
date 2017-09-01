#include "svb_server.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SvB_Server w;
    w.show();

    return a.exec();
}
