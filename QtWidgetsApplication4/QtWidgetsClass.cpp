#include "QtWidgetsClass.h"
#include"WorkerThread.h"
#include"PublishThread.h"
#include<QPainter>
#include"Camera.h"
#include"qtimer.h"
QtWidgetsClass::QtWidgetsClass(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	
	Camera::GetIns()->capture(false);
	QTimer* timer = new QTimer;
	timer->setInterval(20);
	connect(timer, &QTimer::timeout, this, &QtWidgetsClass::slot_timeout);
	timer->start();
	
	PublishThread* pThread = new PublishThread;
	
	pThread->start();
	
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
void QtWidgetsClass::slot_timeout()
{
	Camera::GetIns()->capture(false);
}
void QtWidgetsClass::slot_img(uint32_t* img) {
	//imgData = img;
}