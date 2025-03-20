#pragma once

#include <QWidget>
#include "ui_Camera.h"
#include<qlist.h>
#include<QCamera>
#include<QCameraDevice>
#include<QMediaRecorder>
#include<QMediaCaptureSession>
#include<QImageCapture>
#include<qimage.h>
class Camera : public QWidget
{
	Q_OBJECT

public:
	Camera(QWidget *parent = nullptr);
	~Camera();
	QList<QCameraDevice> list_cameras;
	QScopedPointer<QCamera> my_camera;
	QScopedPointer<QMediaRecorder> my_mediaRecorder;
	QMediaCaptureSession my_captureSession;
	QImageCapture* imageCapture;
	void getCamera();
	bool camera_state;
public slots:
	void imageCaptured(int id, const QImage& preview);
	void on_camera1_btn_clicked(bool);
	void on_capture_btn_clicked(bool);
	
private:
	Ui::CameraClass ui;
};
