#include "Camera.h"
#include<QMediaDevices>
#include<QDir>
#include<qabstractbutton.h>
#include<qbuffer.h>
Camera* Camera::ins = NULL;
Camera::Camera(QObject*parent)
	: QObject(parent)
{
	
    camera_state = false;
    list_cameras = QMediaDevices::videoInputs();
    //�ر�����ͷ
    if (camera_state)
    {
        my_camera->stop();
        camera_state = false;   //��������ͷ״̬
    }

    //�½�����������ͷ
    my_camera.reset(new QCamera(list_cameras[0], NULL));
    //��������ͷ����
    my_captureSession.setCamera(my_camera.data());
    //���ò�׽������ʾ����
    //my_captureSession.setVideoOutput(ui.display_widget);
    //��������ͷ
    my_camera->start();
    //��������ͷ״̬
    camera_state = true;
    //ץ�ľ�ֹͼ��
    imageCapture = new QImageCapture;
    connect(imageCapture, &QImageCapture::imageCaptured, this, &Camera::imageCaptured);
    my_captureSession.setImageCapture(imageCapture);
    
  //  connect(ui.camera1_btn, &QAbstractButton::clicked, this, &Camera::on_camera1_btn_clicked);
}

Camera::~Camera()
{}
Camera* Camera::GetIns()
{
    if (!ins) {
        ins = new Camera();
    }
    return ins;
}
void Camera::getCamera()
{
    

}
//�л�����ͷ1
void Camera::on_camera1_btn_clicked(bool)
{
    
}
void Camera::imageCaptured(int id, const QImage& preview) {
    id++;
    QImage temp = preview;
    temp.convertTo(QImage::Format_RGB888);
    temp= temp.scaled(QSize(960, 540));
    temp.save("wwwww.bmp");
    
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    temp.save(&buffer,"bmp");
    int size = ba.size();
    imgdata = ba;
    return;
}
//ץ��
void Camera::capture(bool)
{
    imageCapture->capture();
   
    return;
    QDir dir;
    //û��photograph�ļ����򴴽�
    if (dir.exists("d:/photograph") == false)
    {
        dir.mkpath("d:/photograph");
    }

    //ʹ��ϵͳʱ��������ͼƬ�����ƣ�ʱ����Ψһ�ģ�ͼƬ��Ҳ��Ψһ��
    QDateTime dateTime(QDateTime::currentDateTime());
    QString time = dateTime.toString("yyyy-MM-dd-hh-mm-ss");

    //����ͼƬ����·����
    QString filename = QDir::currentPath();     //��õ�ǰ·��
    filename += "/photo_";
    filename += QString("%1.jpg").arg(time);

    if (!camera_state)
    {
        //QMessageBox::warning(this, "����", tr("ץ��ʧ�ܣ�����ͷδ�򿪣�"));
        return;
    }
    //QMessageBox::information(this, "��ʾ", tr("ץ�ĳɹ���"));
    imageCapture->captureToFile(filename);      //��ͼ�洢·��
}