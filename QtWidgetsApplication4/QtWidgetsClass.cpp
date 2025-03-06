#include "QtWidgetsClass.h"
#include"WorkerThread.h"
#include<QPainter>
QtWidgetsClass::QtWidgetsClass(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	WorkerThread* thread = new WorkerThread;
	connect(thread, &WorkerThread::sigImg, this, &QtWidgetsClass::slot_img);
	thread->start();
	imgData =(uint32_t*) thread->srcdata;

	
}

QtWidgetsClass::~QtWidgetsClass()
{}
void QtWidgetsClass::paintEvent(QPaintEvent * event)
{
	
		QPainter painter(this);
		QImage img((uchar*)imgData, 960 ,540, QImage::Format_RGB32);
		painter.drawImage(QPoint(0, 0), img);
		update();
	
	
}
void QtWidgetsClass::slot_img(uint32_t* img) {
	//imgData = img;
}