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

    //判断设备数（设置最多两个设备，没有则不会读取设备信息）
    switch (list_cameras.length()) {
    case 1:
        ui.camera1_btn->setText(list_cameras[0].description());
        break;
    case 2:
        ui.camera1_btn->setText(list_cameras[0].description());
        ui.camera2_btn->setText(list_cameras[1].description());
        break;
    }

    if (ui.camera1_btn->text() == "暂无设备")
    {
        ui.camera1_btn->setEnabled(false);
    }
    if (ui.camera2_btn->text() == "暂无设备")
    {
        ui.camera2_btn->setEnabled(false);
    }
}
//切换摄像头1
void Camera::on_camera1_btn_clicked(bool)
{
    //关闭摄像头
    if (camera_state)
    {
        my_camera->stop();
        camera_state = false;   //更新摄像头状态
    }

    //新建并设置摄像头
    my_camera.reset(new QCamera(list_cameras[0], NULL));
    //捕获摄像头画面
    my_captureSession.setCamera(my_camera.data());
    //设置捕捉画面显示窗口
    my_captureSession.setVideoOutput(ui.display_widget);
    //启动摄像头
    my_camera->start();
    //更新摄像头状态
    camera_state = true;
    //抓拍静止图像
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
//抓拍
void Camera::on_capture_btn_clicked(bool)
{
    imageCapture->capture();
    return;
    QDir dir;
    //没有photograph文件夹则创建
    if (dir.exists("d:/photograph") == false)
    {
        dir.mkpath("d:/photograph");
    }

    //使用系统时间来命名图片的名称，时间是唯一的，图片名也是唯一的
    QDateTime dateTime(QDateTime::currentDateTime());
    QString time = dateTime.toString("yyyy-MM-dd-hh-mm-ss");

    //创建图片保存路径名
    QString filename = QDir::currentPath();     //获得当前路径
    filename += "/photo_";
    filename += QString("%1.jpg").arg(time);

    if (!camera_state)
    {
        //QMessageBox::warning(this, "警告", tr("抓拍失败，摄像头未打开！"));
        return;
    }
    //QMessageBox::information(this, "提示", tr("抓拍成功！"));
    imageCapture->captureToFile(filename);      //截图存储路径
}