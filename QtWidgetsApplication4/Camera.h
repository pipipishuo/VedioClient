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
class Camera : public QObject
{
	Q_OBJECT

public:
	Camera(QObject*parent = nullptr);
	~Camera();
	static Camera* ins;
	static Camera* GetIns();
	QList<QCameraDevice> list_cameras;
	QScopedPointer<QCamera> my_camera;
	QScopedPointer<QMediaRecorder> my_mediaRecorder;
	QMediaCaptureSession my_captureSession;
	QImageCapture* imageCapture;
	QByteArray imgdata;
	void getCamera();
	bool camera_state;
public slots:
	void imageCaptured(int id, const QImage& preview);
	void on_camera1_btn_clicked(bool);
	void capture(bool);
	
};
