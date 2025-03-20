#pragma once

#include <QMainWindow>
#include "ui_QtWidgetsClass.h"

class QtWidgetsClass : public QMainWindow
{
	Q_OBJECT

public:
	QtWidgetsClass(QWidget *parent = nullptr);
	~QtWidgetsClass();
	uint32_t* imgData;
protected:
	void paintEvent(QPaintEvent* event);
public slots:
	void slot_img(uint32_t* img);
	void slot_timeout();
private:
	Ui::QtWidgetsClassClass ui;
};
