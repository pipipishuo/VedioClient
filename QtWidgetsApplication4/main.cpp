#include "QtWidgetsApplication4.h"
#include <QtWidgets/QApplication>
#include"QtWidgetsClass.h"
#include"Camera.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtWidgetsClass w;
    w.show();
    
    return a.exec();
}
