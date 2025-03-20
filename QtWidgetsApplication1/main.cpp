#include "QtWidgetsApplication1.h"
#include <QtWidgets/QApplication>
#include"Camera.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Camera c;
    c.show();
   
    /*QtWidgetsApplication1 w;
    w.show();*/
    return a.exec();
}
