#include "Camera.h"
#include<QMediaDevices>
#include<QDir>
#include<qabstractbutton.h>
#include<qbuffer.h>
Camera::Camera(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
    camera_state = false;
    getCamera();
    on_camera1_btn_clicked(false);
    
  //  connect(ui.camera1_btn, &QAbstractButton::clicked, this, &Camera::on_camera1_btn_clicked);
}

Camera::~Camera()
{}
void Camera::getCamera()
{
    list_cameras = QMediaDevices::videoInputs();

    //�ж��豸����������������豸��û���򲻻��ȡ�豸��Ϣ��
    switch (list_cameras.length()) {
    case 1:
        ui.camera1_btn->setText(list_cameras[0].description());
        break;
    case 2:
        ui.camera1_btn->setText(list_cameras[0].description());
        ui.camera2_btn->setText(list_cameras[1].description());
        break;
    }

    if (ui.camera1_btn->text() == "�����豸")
    {
        ui.camera1_btn->setEnabled(false);
    }
    if (ui.camera2_btn->text() == "�����豸")
    {
        ui.camera2_btn->setEnabled(false);
    }
}
//�л�����ͷ1
void Camera::on_camera1_btn_clicked(bool)
{
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
    my_captureSession.setVideoOutput(ui.display_widget);
    //��������ͷ
    my_camera->start();
    //��������ͷ״̬
    camera_state = true;
    //ץ�ľ�ֹͼ��
    imageCapture = new QImageCapture;
    connect(imageCapture, &QImageCapture::imageCaptured, this, &Camera::imageCaptured);
    my_captureSession.setImageCapture(imageCapture);
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
    return;
}
//ץ��
void Camera::on_capture_btn_clicked(bool)
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